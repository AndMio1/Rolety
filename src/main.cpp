/* 
*  Name:	  ArduinoMegaBlinds.ino
*  Created:   09.01.2023
*  Author:	  Andrzej Miozga
*/
#define __SLOG__off      //remove off when you need want to see a log on console
#define __LOGFILE__      //log to file 
#define __MySensor__     //you can use MySensor with SLOG, but MySensor & Debug are not working !!!
#define __WWW__off       //enable www server; Debug and WWW will not work !!!    
#define __BDEBUG__off    //remove off when you need debug from SVcode

#include <Arduino.h>
#include <SPI.h>
#include <USBAPI.h>
#include <SD.h>

#if defined(__BDEBUG__)
	#define	AVR8_SWINT_SOURCE (21)
    #include <avr8-stub.h>
#endif
#include "blindConst.h"
#include "blind.h"

#include <Ethernet_Generic.h>

#if defined(__WWW__)
    char txt[150]     = {0}; 
    #include <EthernetWebServer.h>
    #ifndef BOARD_NAME
	    #define BOARD_NAME BOARD_TYPE
    #endif
#endif    

#if defined(__MySensor__)
    #include <MySensors.h>
#endif

ABlind* BlindArr[NUMBER_OF_BLINDS];
const char* BlindGroups[] = {"/KuchniaSalon", "/GabinetInne", NULL };
//
bool presentationDone = false; // any starting dumy value


int  refreshTime  = 5;
unsigned long watchdogTime = 0;
int PINtoRESET = 54; 

uint8_t mac[] = {MY_MAC_ADDRESS};  // Enter a MAC address and IP address for your controller below.

#if defined(__WWW__)
EthernetWebServer webServer(HTTP_SERVER_PORT); // Initialize the Ethernet server library with the IP address and port you want to use
#endif

//#pragma endregion

void SerialPrintln(const char* input, ...) 
{
    char txtLog[150];
    va_list args;

    va_start(args, input);
    
    #if defined(__SLOG__) || defined(__LOGFILE__)   
    vsprintf(txtLog, input, args);
    #endif
    
    va_end(args);

    #if defined(__SLOG__)
        Serial.println(txtLog);
    #endif
    
    #if defined(__LOGFILE__)   
    File    outfile = SD.open("debuglog.txt", O_WRITE | O_CREAT | O_APPEND);
    if (outfile) 
    {       
        outfile.println(txtLog); 
        outfile.close();   
    }
    #endif    
}

void BlindSetup()
{   
    static bool BlindSetupDone = false;
    if (BlindSetupDone == false)
    {
        BlindSetupDone = true;    

        byte powerPin = PowerPinFirst;
        byte direcPin = DirecPinFirst;

        for (int blind = 0; blind < NUMBER_OF_BLINDS; blind++)
        {
            BlindArr[blind] = new ABlind();
            BlindArr[blind]->BlindSetup(blind+1, powerPin, direcPin, (19 * TIMEFACTOR), (22 * TIMEFACTOR), 0, GroupAll);
        
            powerPin = powerPin + 2;
            direcPin = direcPin + 2;

            SerialPrintln(">>> {Setup} ID:%hd, pos:%hd-%hd %s", blind+1, BlindArr[blind]->Position(), BlindArr[blind]->State(), BlindArr[blind]->Name() );            
        }
    }
}

/// MySensor device presentation, called when modul is active
void presentation()
{
    #if defined(__MySensor__)
    // Send the sketch version information to the gateway and Controller
    BlindSetup();

    sendSketchInfo("Rolety", "2024.1");
    
    for (int blind = 0; blind < NUMBER_OF_BLINDS; blind++)
    {
        // Register all sensors to gw (they will be created as child devices)
        if (present(blind+1, S_COVER, BlindArr[blind]->Name()) == false)
            SerialPrintln(">>> presentation error %s", BlindArr[blind]->Name());
        else
        {
            BlindArr[blind]->sendMessage();
        }
    }
    presentationDone = true;
    #endif
    
    SerialPrintln(">>> {MySensor presentation} done");    
}

//MySensor message incomming
#if defined(__MySensor__)
/// odbiera komunikat z MySensor 
void receive(const MyMessage& message)
{
    byte msgCover  = message.getSensor();
    uint8_t type   = message.getType();
    bool isAck     = message.isAck();
    bool isEcho    = message.isEcho();
    byte pct       = message.getByte();
    mysensors_command_t cmd = message.getCommand();   

    //SerialPrintln(">>> {recieved}: id: %hd  type: %hd: Pos: %hd ack: %hd echo: %hd cmd: %hd", msgCover, type, pct, isAck, isEcho, cmd);    
    if (isEcho == false && isAck == false && cmd != C_INTERNAL && msgCover && msgCover < NUMBER_OF_BLINDS)
    {
        msgCover = msgCover-1;
        switch (type)
        {
            case V_UP: //29
                BlindArr[msgCover]->setOpen();
                break;

            case V_DOWN: //30
                BlindArr[msgCover]->setClose();
                break;

            case V_STOP: //31
                BlindArr[msgCover]->setStop();
                break;

            case V_STATUS:
                if (pct == ABlindData::OpenedPos)
                    BlindArr[msgCover]->setOpen();
                else
                if (pct == ABlindData::ClosedPos)
                    BlindArr[msgCover]->setClose();
                else
                    BlindArr[msgCover]->setPosition(pct);
                break;

            case V_PERCENTAGE: //3
                BlindArr[msgCover]->setPosition(pct);
                break;
        }
    }

    watchdogTime = millis(); //it recives message.getCommand() != C_INTERNAL, what is information that it is alive
}
#endif


#if defined(__WWW__)
//WEB SERVER

void handleRootPage()
{
    String html;
    html = F("<!DOCTYPE HTML><head>"
             //"<meta http-equiv='refresh' content='1'>"
             "</head><html>"
             "<head>"
             "<meta name='viewport' content='width=device-width, initial-scale=2.0'>"      
             "<TITLE>Rolety</TITLE>"
             "</head>"
             "<h3>Grupy rolet</h3>");
    for(int i=0; i >= 0; i++)
    {
        if (BlindGroups[i] != NULL)
        {
            txt[0] = 0;
            sprintf(txt, "<br><a href='%s'>%s</a>", BlindGroups[i], BlindGroups[i]);
            html += txt;
        }
        else
            break;
    }
    //html += F("</html>\r\n");
    unsigned long x = millis() / 1000;
    int s = x % 60;
    int m = (x / 60) % 60;
    int h = x / 3600;
    sprintf(txt, "<br>%d:%d:%d</html>", h,m,s);

    html += txt;
    webServer.send(200, F("text/html"), html);

    watchdogTime = millis();    
}

void handleRunPage()
{    
    BlindState  reqStatus   = BlindState::noInit;
    int         newPos      = -1;
    int         pos;
    String      argName;
    String      rediretTo = "/";
    String      html;

    if(webServer.hasArg("URI"))
        rediretTo = webServer.arg("URI");

    if (webServer.hasArg("DW"))
        reqStatus = BlindState::moveDown;
    else
    if (webServer.hasArg("UP"))
        reqStatus = BlindState::moveUp;
    else
    if (webServer.hasArg("ST"))
        reqStatus = BlindState::Middle;
    else
    if (webServer.hasArg("POS"))
    {
        String p = webServer.arg("POS");
        if (p.length())
        {
            newPos = webServer.arg("POS").toInt();
            if (newPos < 0)   newPos = ABlindData::ClosedPos;        
            if (newPos > 100) newPos = ABlindData::OpenedPos;        
            reqStatus = BlindState::Middle; 
        }
    }
    if (reqStatus != BlindState::noInit)
        refreshTime = 2;

    for (int blind = 1; blind <= NUMBER_OF_BLINDS; blind++)
    {       
        pos = BlindArr[blind - 1]->Position();
    
        argName = F("S");  
        argName += String(blind);;
        if (newPos >=0 && webServer.hasArg(argName) && newPos != pos)
        {
            BlindArr[blind - 1]->setPosition(newPos);
            BlindArr[blind - 1]->setSelected(true);
        }
        else
        if (newPos == -1 && reqStatus != BlindState::noInit && webServer.hasArg(argName) )
        {
            if (reqStatus == BlindState::Middle)   BlindArr[blind - 1]->setStop();
            else
            if (reqStatus == BlindState::moveDown) BlindArr[blind - 1]->setClose();
            else
            if (reqStatus == BlindState::moveUp)   BlindArr[blind - 1]->setOpen();

            BlindArr[blind - 1]->setSelected(true);
        }
        else
            BlindArr[blind - 1]->setSelected(false);        
    }
    html = F(   "<html><head>"
                "<TITLE>Rolety</TITLE>"
                "</head>"
                "<body>"
                "-GO-"        
                "</body></html>"); 
    webServer.sendHeader(F("Cache-Control"), F("no-cache"), true);       
    webServer.sendHeader(F("Location"), rediretTo);    
    webServer.send(301, F("text/html"), html);

    watchdogTime = millis();        
}

void handleMainPage()
{
    BlindState  state       = BlindState::noInit;
    BlindState  reqStatus   = BlindState::noInit;
    int         pos;
    String      sblind;
    boolean     selected;
    String      html; 
    String      grp = webServer.uri();

    html = F("<!DOCTYPE HTML><html><head>");
    //html += F("<meta charset='UTF-8'>");
    if (refreshTime)
    {
        sprintf(txt, "<meta http-equiv='refresh' content='%d'>", refreshTime);
        html += txt;
    }
    
    html += F(  "<meta name='viewport' content='width=device-width, initial-scale=2.0'>"       
                "<TITLE>Rolety</TITLE>"
                "<style>table{font-family: arial; border-collapse: collapse}td,th{border: 0px;text-align: center;padding: 4px;} button{font-size:22px}</style>"
                "</head>"
                "<body><form>"
                "<table><tr>"
                "<th></th><th>Nazwa</th><th>Poz</th></tr>");
    for (int blind = 1; blind <= NUMBER_OF_BLINDS; blind++)
    {       
        if (BlindArr[blind - 1]->isGroup(grp))
        {
            sblind   = String(blind);
            pos      = BlindArr[blind - 1]->Position();
            state    = BlindArr[blind - 1]->State();
            selected = BlindArr[blind - 1]->isSelected();
            if (selected && state != BlindState::moveDown && state != BlindState::moveUp)
            {
                selected = false;
                BlindArr[blind - 1]->setSelected(false);
            }
            sprintf(txt,"<tr><td><input type='checkbox' name='S%d'%s></td><td>%s</td><td>", blind, (selected ? " checked" : " "), BlindArr[blind - 1]->Name());
            html += txt;            
            //html += F("<tr><td><input type='checkbox' name='S"); 
            //html += sblind; 
            //html += (selected ? F("' checked></td>") : F("'></td>"));
            //html += F("<td>"); 
            //html += BlindArr[blind - 1]->Name();  
            //html += F("</td><td>");
            if( pos == ABlindData::OpenedPos )
                sprintf(txt," %s </td></tr>", " &#x25AF");
            else if (pos == ABlindData::ClosedPos)            
                sprintf(txt," %s </td></tr>", " &#x25AE");
            else                
                sprintf(txt,"%s %d </td></tr>", (state == BlindState::moveDown ? " &#x2191 " : (state == BlindState::moveUp ? " &#x2193 " : "" )) ,pos);
            html += txt;            
            /*            
            if (state == BlindState::moveDown) html += F(" &#x2191 ");
            else
            if (state == BlindState::moveUp)   html += F(" &#x2193 ");

            if (pos == ABlindData::OpenedPos)  html += F(" &#x25AF");  
            else
            if (pos == ABlindData::ClosedPos)  html += F(" &#x25AE");  
            else                               html += String(pos);                 
            html += F("</td></tr>");    
            */
            if (reqStatus == BlindState::noInit && (state == BlindState::moveUp || state == BlindState::moveDown))
                reqStatus = BlindState::Middle;
        }
    }
    
    html += F("<tr><td colspan=3><button type='submit' name='UP' formaction='/run'>&#x21E7</button>"
              "   <button type='submit' name='ST' formaction='/run'>&#x2297</button>"
              "   <button type='submit' name='DW' formaction='/run'>&#x21E9</button></td>"
              "</tr><tr>"
              "<td colspan=3><input type='number' max=100 name='POS' style='width:40px'>"
              " <input type='submit' value='&#x25BA' formaction='/run'>   <a href='/'>[home]</a></td>"
              "</tr></table>");       
    //html += F("<input hidden name='URI', value='"); html += webServer.uri();  html += F("'>");        
    //html += F("<form></body></html>");
    sprintf(txt,"<input hidden name='URI', value='%s'> <form></body></html>", webServer.uri().c_str());
    html += txt; 
    //SerialPrintln(">>> {handleMainPage} page len %d", html.length());        
    //}
    //Save2File(html);

    webServer.sendHeader(F("Cache-Control"), F("no-cache"), true);
    webServer.send(200, "text/html", html);
    
    if (reqStatus == BlindState::noInit)
    {   //next refresh time
        refreshTime = 30;
    }

    watchdogTime = millis();          
}

void handleNotFound()
{
    String message = F("Page Not Found\n\n");

    message += F("URI: ");
    message += webServer.uri();
    message += F("\nMethod: ");
    message += (webServer.method() == HTTP_GET) ? F("GET") : F("POST");
    message += F("\nArguments: ");
    message += webServer.args();
    message += F("\n");

    for (uint8_t i = 0; i < webServer.args(); i++)
    {
        message += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
    }

    webServer.send(404, F("text/plain"), message);
}

#endif

//usedby MySensor but not exclusivelly, can be exetuted once
void before()
{
    static bool beforeDone = false;
    if (!beforeDone)
    {
        pinMode(PINtoRESET, INPUT);    // Just to be clear, as default is INPUT. Not really needed.
        digitalWrite(PINtoRESET, LOW);
    
    
        beforeDone = true;            
        #if defined(__SLOG__)    
        Serial.begin(115200);
        while (!Serial && millis() < 5000)
        { 
            delay(1000); // wait for serial port to connect. Needed for native USB port only        
        }
        //SerialPrintln(">>> Serial is on");            
        #endif

        pinMode(SD_USE_THIS_PIN, OUTPUT);
        digitalWrite(SD_USE_THIS_PIN, HIGH);
        if (!SD.begin(SD_USE_THIS_PIN)) 
        {
            SerialPrintln(">>> SD initialization failed");        
            //while (1);
            //If an SD card is inserted but not used, it is possible for the sketch to hang, because pin 4 is used as SS (active low) of the SD and when not used it is configured as INPUT by default. 
        } 
        //else 
        //{    
        //    SerialPrintln(">>> SD Wiring is correct and a card is present.");
        //}
    }
}

//Arduino initialization
void setup()
{          
    before();

    // start the Ethernet connection and the server:
    SerialPrintln(">>> SETUP: %s", SHIELD_TYPE);
    // You can use Ethernet.init(pin) to configure the CS pin        
    //#if !defined(__MySensor__)   
    Ethernet.init();
    Ethernet.begin(mac);
    //#endif        
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
        SerialPrintln("Ethernet shield was not found.  Sorry, can't run without hardware.");
        while (true) 
        {
            delay(100); // do nothing, no point running without Ethernet hardware
        }
    }
    if (Ethernet.linkStatus() == LinkOFF)
    {
        SerialPrintln("Ethernet cable is not connected.");        
        while (Ethernet.linkStatus() == LinkOFF) 
        {
            delay(1000); // do nothing, no point running without Ethernet hardware
        }
    }
    //IPAddress ip = Ethernet.localIP();    
    //SerialPrintln("Ethernet IP: %hd.%hd.%hd.%hd", ip[0], ip[1], ip[2], ip[3]);    

    #if defined(__BDEBUG__)
        debug_init();
    #endif
    SerialPrintln(">>> SETUP: done");    
}

/// petla aktualizujaca stan rolet, kontroluje czas ruchu, moment startu czy zatrzymania oraz wysyłająca powiadomienie do mySensor
void blindLoop()
{
    bool alive = false;
    static unsigned long aliveMs = 0; 

    for (int blind = 0; blind < NUMBER_OF_BLINDS; blind++)
    {
        BlindArr[blind]->doLoop();
        #if defined(__MySensor__)
        if (presentationDone)
        {
            alive = alive || BlindArr[blind]->sendMessage();
        }
        #endif
    }
    #if defined(__MySensor__)
        if (alive == false)
        {
            unsigned long x = millis();
            if (x > aliveMs)
                x = x - aliveMs;
            if ( x > 10000)
            {            
                sendHeartbeat();
                aliveMs = millis();
            }
        }
        else
            aliveMs = millis();
    #endif
}

bool blindInit()
{
    static bool blindInitDone = false;  
    if (blindInitDone == false)
    {
        blindInitDone = true;
        BlindSetup();
        delay(500);

        // start the server
        #if defined(__WWW__)                             
            webServer.on("/", handleRootPage);
            webServer.on("/run", handleRunPage);
            webServer.onNotFound(handleNotFound);
            for(int i=0; i >= 0; i++)
            {
                if (BlindGroups[i] != NULL)
                {            
                    webServer.on(BlindGroups[i], handleMainPage);                    
                    //SerialPrintln(">>> WebServer - %s", BlindGroups[i]);
                }
                else
                    break;
            }
            webServer.begin();                 
        #endif

    }
    return blindInitDone;
}

void Watchdog()
{   //ethernet freezing overcome    
    unsigned long now = millis();
    if (now >= watchdogTime)
        now = (now - watchdogTime) / 1000;
    else
        now = (now + (ULONG_MAX - watchdogTime))/1000;
    
    if (now / 60)
    {
        SerialPrintln(">>> {Watchdog} reset1 %ld", millis());
        delay(LOOP_DELAY);
        
        Ethernet.hardreset();        
    }
    
    if (now / 120)
    {
        SerialPrintln(">>> {Watchdog} reset2 %ld", millis());          
        delay(LOOP_DELAY);

        pinMode(PINtoRESET, OUTPUT);   // lights out. Assuming it is jumper-ed correctly.
    }    
}

bool sendMyMessage(byte _id, byte _position, bool _stop)
{    
    bool result = false;        
    #if defined(__MySensor__) 
        if (_stop)
        {
            MyMessage coverMsg(_id, V_UP);
            send(coverMsg.set((boolean)(_position == ABlindData::OpenedPos)));
            //SerialPrintln(">>> Send message V_UP: %hd - pos: %hd", _id, _position);    
            result = true;        
        }
        if (_stop)
        {
            MyMessage coverMsg(_id, V_DOWN);
            send(coverMsg.set((boolean)(_position == ABlindData::ClosedPos)));
            //SerialPrintln(">>> Send message V_DOWN: %hd - pos: %hd", _id, _position);   
            result = true;             
        }
        if (1)
        {
            MyMessage coverMsg(_id, V_PERCENTAGE);
            send(coverMsg.set(_position));
            //SerialPrintln(">>> Send message V_PREC: %hd - pos: %hd", _id, _position);
            result = true;
        }
        if(_stop)
        {
            MyMessage coverMsg(_id, V_STOP);
            send(coverMsg.set(_stop));
            //SerialPrintln(">>> Send message V_STP: %hd - pos: %hd", _id, _position);
            result = true;
        }                
    #endif     
    return result;
}
//Arduino main loop
void loop() 
{  
    // listen for incoming clients
    if (blindInit())
    {               
        #if defined(__WWW__)
        webServer.handleClient();
        #endif       

        blindLoop();
    }
    Watchdog();
}
