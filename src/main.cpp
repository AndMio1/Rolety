/*
 Name:		ArduMegaBlinds.ino
 Created:	09.01.2021 16:39:30
 Author:	Andrzej
*/
#include "blindConst.h"
#include <SPI.h>
#include <Ethernet.h>
#include <MySensors.h>
#include <MyConfig.h>
#include "blind.h"
//#include <stdarg.h>

//#pragma region WebServerVariables
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
// IPAddress ip(10, 10, 20, 122);
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Initialize the Ethernet server library with the IP address and port you want to use
EthernetServer webServer(80);

//#pragma endregion WebServerVariables

//#pragma region MyVariables
//unsigned long stime;

ABlind BlindArr[NUMBER_OF_BLINDS];
//  in secounds                           1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
byte timeTableUp[NUMBER_OF_BLINDS]   = { 22, 22, 22, 22, 22 };// , 40, 40, 40, 40, 40, 40, 40, 40, 40, 40 };
byte timeTableDown[NUMBER_OF_BLINDS] = { 22, 22, 22, 22, 22 };// , 40, 40, 40, 40, 40, 40, 40, 40, 40, 40 };
byte presentationDone = -2;

char msg[128];

//#pragma endregion

void SerialPrintln(const char* input, ...) 
{
    char txt[255]={0};
    va_list args;
    va_start(args, input);
    
    vsprintf(txt, input, args);
    va_end(args);
    //Serial.println(millis());
    Serial.println(txt);
}

//MySensor initializaton
void before()
{
    byte powerPin = PowerPinFirst;
    byte direcPin = DirecPinFirst;
    
    SerialPrintln(">>> Before");

    for (int blind = 0; blind < NUMBER_OF_BLINDS; blind++)
    {
        BlindArr[blind].BlindSetup(blind+1, powerPin, direcPin, timeTableUp[blind], timeTableDown[blind]);
        powerPin = powerPin + 2;
        direcPin = direcPin + 2;
    }
}

//MySensor device presentation
void presentation()
{
    SerialPrintln(">>> Presentation");    
    // Send the sketch version information to the gateway and Controller
    sendSketchInfo("Relay", "1.1", true);

    for (int blind = 0; blind < NUMBER_OF_BLINDS; blind++)
    {
        // Register all sensors to gw (they will be created as child devices)
        sprintf(msg, "Roleta %d", blind+1);
        present(blind+1, S_COVER, msg);
    }
    presentationDone = 0;    
}

//MySensor message incomming
void receive(const MyMessage& message)
{
    byte msgCover  = message.getSensor();
    uint8_t type   = message.getType();
    bool isAck     = message.isAck();
    bool isEcho    = message.isEcho();
    //uint8_t sender = message.getSender();
    byte pct       = message.getByte();
   
    if (isEcho == false && isAck == false)
    {
        switch (type)
        {
        case V_UP: //29
            BlindArr[msgCover - 1].setOpen();
            break;

        case V_DOWN: //30
            BlindArr[msgCover - 1].setClose();
            break;

        case V_STOP: //31
            BlindArr[msgCover - 1].setStop();
            break;

        case V_PERCENTAGE: //3
            BlindArr[msgCover - 1].setPosition(pct);
            break;
        }
    }
    SerialPrintln(">>> Message Recieved: id: %d  type: %d: Pos: %d ack: %d echo: %d", msgCover, type, pct, isAck, isEcho);    
}

//Arduino initialization
void setup()
{
    IPAddress ipAdr;
    // You can use Ethernet.init(pin) to configure the CS pin
    //Ethernet.init(10);  // Most Arduino shields
    //Ethernet.init(5);   // MKR ETH shield
    //Ethernet.init(0);   // Teensy 2.0
    //Ethernet.init(20);  // Teensy++ 2.0
    //Ethernet.init(15);  // ESP8266 with Adafruit Featherwing Ethernet
    //Ethernet.init(33);  // ESP32 with Adafruit Featherwing Ethernet

    // Open serial communications and wait for port to open:
    presentationDone = -1;
    Serial.begin(9600);
    while (!Serial) 
    { 
        sleep(100); 
    } // wait for serial port to connect. Needed for native USB port only        
    
    SerialPrintln(">>> SETUP");

    // start the Ethernet connection and the server:
    Ethernet.begin(mac);
    
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
        SerialPrintln("Ethernet shield was not found.  Sorry, can't run without hardware.");
        while (true) {
            delay(100); // do nothing, no point running without Ethernet hardware
        }
    }

    if (Ethernet.linkStatus() == LinkOFF)
    {
        SerialPrintln("Ethernet cable is not connected.");
        
        while (true) {
            delay(100); // do nothing, no point running without Ethernet hardware
        }
    }
    ipAdr = Ethernet.localIP();

    SerialPrintln("Ethernet WebServer: %d.%d.%d.%d", ipAdr[0], ipAdr[1], ipAdr[2], ipAdr[3]);
    
    // start the server
    webServer.begin();    
/*
    //Retreive our last light state from the eprom
    int LightState=loadState(EPROM_LIGHT_STATE);
    if (LightState<=1) 
    {
        LastLightState=LightState;
        int DimValue=loadState(EPROM_DIMMER_LEVEL);
        if ((DimValue>0)&&(DimValue<=100)) 
        {
            //There should be no Dim value of 0, this would mean LIGHT_OFF
            LastDimValue=DimValue;
        }
*/
}


void blindLoop()
{
    for (int blind = 0; blind < NUMBER_OF_BLINDS; blind++)
    {
        if (presentationDone == 1)
        {
            BlindArr[blind].doLoop();
            BlindArr[blind].sendMessage();
        }
    }
}

void blindInit()
{
    SerialPrintln(">>> BlindInit");
    for (int blind = 0; blind < NUMBER_OF_BLINDS; blind++)
    {
        BlindArr[blind].setOpen();
    }
}

void sendMyMessage(byte _id, byte _position)
{
    if (_position == BLIND_OPEN)
    {
        MyMessage coverMsg(_id, V_UP);
        send(coverMsg.set((boolean)true));
        //position = 0;
        SerialPrintln(">>> Send message: %d - pos: %d", _id, _position);            
    }
    else
    {
        if (_position == BLIND_CLOSED)
        {
            MyMessage coverMsg(_id, V_DOWN);
            send(coverMsg.set((boolean)true));
            //position = 100;
            SerialPrintln(">>> Send message: %d - pos: %d", _id, _position);                
        }
        else
        {
            MyMessage coverMsg(_id, V_PERCENTAGE);
            send(coverMsg.set(_position));
        }
    }    
}


void clientPageBuild(EthernetClient client)
{
    String     requestString = "";    
    byte       reqBlindIdx;
    BlindState reqStatus = BlindState::noInit;

    if (client)
    {
        // an http request ends with a blank line
        boolean currentLineIsBlank = true;
        while (client.connected()) 
        {
            if (client.available()) 
            {
                char c = client.read();
                if (requestString.length() < 100)
                    requestString += c;                                

                if (c == '\n' && currentLineIsBlank)
                {
                    BlindState state = BlindState::noInit;

                    if (currentLineIsBlank)
                    {            
                        int pos = requestString.indexOf("GET /?button");
                        int idx;    
                        if (pos >= 0)
                        {
                            pos = 5+pos + strlen("?button");                
                            idx = atoi( requestString.substring(pos, pos+2).c_str() );
                            if (idx > 0)
                            {
                                reqBlindIdx = idx;
                                if (requestString.indexOf("up") > pos)
                                    reqStatus = BlindState::moveUp;                                    
                                else
                                if (requestString.indexOf("dn") > pos)
                                    reqStatus = BlindState::moveDown;
                                else    
                                if (requestString.indexOf("st") > pos)
                                    reqStatus = BlindState::Middle;
                            }    
                            SerialPrintln("#debug request# pos %d, idx %d", pos, idx);
                        }
                    }

                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: text/html");
                    client.println("Connection: close");  // the connection will be closed after completion of the response
                    //ato refrehs when moving 
                    for (int blind = 1; blind <= NUMBER_OF_BLINDS; blind++)
                    {
                        state = BlindArr[blind - 1].State();
                        if (state == BlindState::moveDown || state == BlindState::moveUp )
                        {
                            break;
                        }
                    }
                    if (state == BlindState::moveDown || state == BlindState::moveUp || reqStatus != BlindState::noInit)
                        client.println("Refresh: 1; url=\\");  // refresh the page automatically every 5 sec                                
                    else                        
                        client.println("Refresh: 30; url=\\");  // refresh the page automatically every 5 sec                                
                    
                    client.println();
                    client.println("<!DOCTYPE HTML>");
                    client.println("<html>");
                    // output the value of each analog input pin
                    client.println("<HEAD>");
                    client.println("<meta name='apple-mobile-web-app-capable' content='yes' />");
                    client.println("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");
                    //client.println("<link rel='stylesheet' type='text/css' href='https://randomnerdtutorials.com/ethernetcss.css' />");
                    client.println("<link rel='stylesheet' href='https://ssl.gstatic.com/docs/script/css/add-ons1.css' />");
                    client.println("<link href='https://fonts.googleapis.com/icon?family=Material+Icons'   rel='stylesheet' />");

                    client.println("<style>table, th, td { border: 1px solid black; }  th, td { padding: 2px; } table.center { margin-left: auto; margin-right: auto;} </style>");

                    client.println("<TITLE>Rolety</TITLE>");
                    client.println("</HEAD>");
                    client.println("<BODY>");
                    client.println("<hr />");
                    client.println("<table>  <tr> ");
                    client.println("<th>Gora</th> <th>Stop</th> <th>Dol</th>  <th>Poz</th> ");
                    client.println("</tr>");
                    for (int blind = 1; blind <= NUMBER_OF_BLINDS; blind++)
                    {
                        char str[128];

                        //client.print( BlindArr[blind - 1].getStateStr() );
                        client.println("<tr>");    
                        sprintf(str, "<td> <a href=\"/?button%02dup\"\"> <span class='material-icons'>keyboard_double_arrow_up </span>    </a> </td>", blind);  client.println(str);
                        sprintf(str, "<td> <a href=\"/?button%02dst\"\"> <span class='material-icons'>stop</span>                         </a> </td>", blind);  client.println(str);
                        sprintf(str, "<td> <a href=\"/?button%02ddn\"\"> <span class='material-icons'>keyboard_double_arrow_down</span>   </a> </td>", blind);  client.println(str);
                        sprintf(str, "<td> <input type='text' id='Pos%02d' style='width: 50px;' value='%02d' maxlen=2> </td>", blind,  BlindArr[blind - 1].Position());  client.println(str);

                        client.println("</tr>");    
                        //client.println("<br />");
                    }
                    client.println("</table>");

                    client.print("CMD: ");
                    client.print(msg);
                    client.println("<br />");
                    client.println("</BODY>");
                    client.println("</html>");
                    break;
                }
                if (c == '\n') {
                    // you're starting a new line
                    currentLineIsBlank = true;
                }
                else if (c != '\r') {
                    // you've gotten a character on the current line
                    currentLineIsBlank = false;
                }
            }
        }
        // give the web browser time to receive the data
        
        client.stop();
        if (currentLineIsBlank && reqStatus != BlindState::noInit)
        {            
            if (reqBlindIdx > 0)
            {
                SerialPrintln("#debug request ACT# pos %d, action %d", reqBlindIdx, reqStatus);                
                if (reqStatus == BlindState::moveUp)
                    BlindArr[reqBlindIdx - 1].setOpen();

                if (reqStatus == BlindState::Middle)
                    BlindArr[reqBlindIdx - 1].setStop();                

                if (reqStatus == BlindState::moveDown)
                    BlindArr[reqBlindIdx - 1].setClose();

                //Serial.println(txt);
                Serial.println(">>> ----");
                Serial.println(requestString.c_str());
                Serial.println("<<< ----");
            }    
        }
    }
}
//Arduino main loop
void loop() 
{
    // listen for incoming clients
    EthernetClient client = webServer.available();

    if (presentationDone == 0) //after method presentation was called
    {
        presentationDone = 1;        
        blindInit();
        
    }
    blindLoop();
    clientPageBuild(client);
}