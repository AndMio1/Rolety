#include <Arduino.h>
#include "blindConst.h"
#include "blind.h"

#include <SD.h>

ABlind::ABlind()
{
    
    offset           = 0;
    started          = 0;
    state = BlindState::noInit;
}

void ABlind::BlindSetup(byte _ID, byte _pinPowerOn, byte _pinDirection, unsigned long _timeUp, unsigned long _timeDown, byte _position, const char* _group)
{
    ABlindData localData;
    offset      = 0;
    started     = 0;
    sendUpdate  = true;
    data.position            = _position;
    data.ID                  = _ID;
    data.parmPinPowerOn      = _pinPowerOn;
    data.parmPinDirection    = _pinDirection;
    data.parmTimeUp          = _timeUp; //milisekundy
    data.parmTimeDown        = _timeDown;
    data.name = F("Roleta");
    data.name += String(data.ID);
    if (strlen(_group) == 0)
    {
        data.group = GroupAll;
    }
    localData.ID = _ID;
    if (localData.readCfg())
    {
        data.Init(localData);
    }
    data.position = this->validatePosition(data.position);

    switch (data.position)
    {
        case ABlindData::ClosedPos:
            state = BlindState::stopedDown;
        break;

        case ABlindData::OpenedPos:
            state = BlindState::stopedUp;
        break;

        default:
            state = BlindState::Middle;
    }

    if (data.parmPinPowerOn)
    {
        digitalWrite(data.parmPinPowerOn, HIGH);   //wyłącz sterowanie / zasilanie, stan domyślny wyjść    
        pinMode(data.parmPinPowerOn, OUTPUT);
        digitalWrite(data.parmPinPowerOn, HIGH);   //wyłącz sterowanie / zasilanie, stan domyślny wyjść    
    }
    if (data.parmPinDirection)
    {
        digitalWrite(data.parmPinDirection, HIGH);   //kierunek góra - stan domyślny wyjść    
        pinMode(data.parmPinDirection, OUTPUT);
        digitalWrite(data.parmPinDirection, HIGH);   //kierunek góra - stan domyślny wyjść    
    }
}

void ABlind::SetDirection(boolean _downUp)
{
    if (data.parmPinDirection)
    {
        if (_downUp == false)
            digitalWrite(data.parmPinDirection,  LOW);  // kierunek dół  - przekażnik załączony
        else
            digitalWrite(data.parmPinDirection,  HIGH); // kierunek góra - przekażnik wyłączony,stan domyślny wyjść            
    }
}

void ABlind::SetPower(boolean _offOn)
{
    if (data.parmPinPowerOn)
    {
        if (_offOn)
        {
            delay(RELAY_DELAY);    
            digitalWrite(data.parmPinPowerOn, LOW);      //włącz sterowanie
        }
        else
        {
            digitalWrite(data.parmPinPowerOn,    HIGH);  //wyłącz sterowanie - przekaźnik niezasilony
            delay(RELAY_DELAY);
            digitalWrite(data.parmPinDirection,  HIGH);  // kierunek góra - przekażnik wyłączony,stan domyślny wyjść
        }        
    }
}

byte ABlind::validatePosition(int8_t _percent)
{
    byte percent = (_percent % 100);
    if (_percent >= ABlindData::OpenedPos)
        percent = ABlindData::OpenedPos;
    else
        if (_percent <= ABlindData::ClosedPos)
            percent = ABlindData::ClosedPos;

    return percent;
}

void ABlind::setStop(bool _powerOff) // _powerOff = true //natychmiastowe zatrzymanie
{
    unsigned long time = elapsedTime();
    int8_t        pos  = data.position+offset;

    if (_powerOff == true)
        SetPower(false);  //off

    SerialPrintln(" Blind stop ID: %d, %d, %d", data.ID, data.position, state);     

    if (started > 0 && expectedTime > 0)
    {   
        //calculate the current position
        if (state == BlindState::moveDown)
        {
            pos = (int8_t)(time * (unsigned long)100 / data.parmTimeDown);
            data.position = validatePosition((int8_t)(data.position + pos));
        }
        if (state == BlindState::moveUp)
        {
            pos = (int8_t)(time * (unsigned long)100 / data.parmTimeUp);
            data.position = validatePosition((int8_t)(data.position - pos));
        }
    }
    started      = 0;
    expectedTime = 0;
    offset       = 0;

    switch (data.position)
    {
        case ABlindData::ClosedPos:
            state = BlindState::stopedDown;
        break;

        case ABlindData::OpenedPos :
            state = BlindState::stopedUp;
        break;

        default:
            state = BlindState::Middle;    
    }

    sendUpdate = true;

    data.writeCfg();
}

void ABlind::setClose()    //procent 100, na ile ustawić a nie o ile przestawić
{
    SerialPrintln(">>> Blind close ID:%d (pos: %d)", data.ID, data.position);
    setPosition(ABlindData::ClosedPos);
}

void ABlind::setOpen()     //procent 0, na ile ustawić a nie o ile przestawić
{
    SerialPrintln(">>> Blind open: ID:%d (pos: %d)", data.ID, data.position);
    setPosition(ABlindData::OpenedPos);
}

void ABlind::setPosition(byte _percent) //procent 0-100, 100 to zamkniete
{
    byte newPos = validatePosition(_percent);
    BlindState  newState = BlindState::noInit;
    
    if ((data.position+offset) < newPos)
        newState  = BlindState::moveDown;
    else        
        if ((data.position+offset) > newPos)
            newState  = BlindState::moveUp;
    
    setStop(newState != state);

    if (data.position < newPos)
    {
        state  = BlindState::moveDown;
        newPos = newPos - data.position;
        
        expectedTime = (unsigned long)newPos * data.parmTimeDown / 100;
        SetDirection(true);//góra

        SerialPrintln(">>> Blind mUP ID:%d, %d, %d", data.ID, data.position, _percent);
    }
    else
        if (data.position > newPos)
        {
            state  = BlindState::moveUp;
            newPos = data.position - newPos;
            
            expectedTime = (unsigned long)newPos * data.parmTimeUp / 100;
            SetDirection(false);//dół

            SerialPrintln(">>> Blind mDOWN ID: %d, %d, %d", data.ID, data.position, _percent);            
        }

    if (state == BlindState::moveDown || state == BlindState::moveUp)
    {
        started = millis();
        SetPower(true);     //on
    }
    sendUpdate = true;
}


byte ABlind::doLoop()
{
    unsigned long time;
    int8_t newOffset;

    if (state == BlindState::moveDown || state == BlindState::moveUp)
    {
        time = elapsedTime();
        
        if (time >= expectedTime)            
        {
            SerialPrintln("  Stop1 ID: %d, %ld, %ld", data.ID, time, expectedTime);
            setStop();
            newOffset = offset;
        }
        else
        {
            newOffset = offset;
            //oblicz pozycję poprzez - jaki procent czasu upłynął
            if (state == BlindState::moveDown)
                offset = (int8_t)(100 * time / data.parmTimeDown);                
            else
                offset = -(int8_t)(100 * time / data.parmTimeUp);                
        }

        if (newOffset != offset)
        {
            sendUpdate = true;
            if (validatePosition(data.position+offset) == ABlindData::ClosedPos || validatePosition(data.position+offset) == ABlindData::OpenedPos)
            {
                SerialPrintln("  Stop2 ID: %d, %ld, %ld", data.ID, data.position+offset, newOffset - offset);                
                setStop();
            }
        }
    }
    return (data.position+offset);
}

unsigned long ABlind::elapsedTime()
{
    unsigned long time = 0;
    unsigned long now = millis();    

    if (now >= started)
        time = now - started;
    else
        time = now + (ULONG_MAX - started);

    return time;
}

bool ABlind::sendMessage()
{
    if (sendUpdate)
    {
        sendUpdate = false;
        return sendMyMessage(data.ID, data.position+offset, offset == 0);        
    }
    return false;
}

byte ABlind::Position()
{
    return validatePosition(data.position+offset);
}

BlindState ABlind::State()
{
    return state;
}

const char*  ABlind::Name()
{
    return data.name.c_str();
}

const char* ABlind::Group()
{
    return data.group.c_str();
}

bool ABlind::isGroup(String& _group)
{
    return data.group.compareTo(_group) == 0;
}

void ABlind::setSelected(bool _selected)
{
    selected = _selected;
}

bool ABlind::isSelected()
{
    return selected;
}


//###############################################################################################
String  ABlindData::GetFileName(byte _id)
{
    String  filename("R"); 
    filename += String(_id); filename += F(".txt");
    return filename;
}

void ABlindData::writeCfg()
{
    File    outfile;
    String  buf;
    int     fileLen = 0;
    
    outfile = SD.open(GetFileName(ID), O_WRITE | O_CREAT);
    if (outfile) 
    {
        outfile.seek(0);
        outfile.flush();
        //1
        buf = String(ID);    
        fileLen += buf.length();
        outfile.println(buf);
        //2
        buf = String(position);    
        fileLen += buf.length();
        outfile.println(buf);
        //3
        buf = String(parmTimeUp);    
        fileLen += buf.length();
        outfile.println(buf);
        //4
        buf = String(parmTimeDown);    
        fileLen += buf.length();
        outfile.println(buf);
        //5
        buf = String(parmPinPowerOn);    
        fileLen += buf.length();
        outfile.println(buf);
        //6
        buf = String(parmPinDirection);    
        fileLen += buf.length();
        outfile.println(buf);
        // >>
        //7
        fileLen += name.length();
        outfile.println(name);
        //8
        fileLen += group.length();
        outfile.println(group);
        // <<
        //9 sum lenght
        buf = String(fileLen);
        outfile.println(buf);
        outfile.close();
    }  
    else
    {
        SerialPrintln(">>> Error writing a file %s", GetFileName(ID).c_str());        
    }
}

bool ABlindData::readCfg()
{
    File    outfile;
    String  buf;
    int     fileLen = 0;

    outfile = SD.open(GetFileName(ID));
    if (outfile)
    {        
        int step = 1;
        while (outfile.available()) 
        {
            buf = outfile.readStringUntil('\n');
            buf.trim();
            switch (step)
            {
                case 1:
                    ID = buf.toInt();
                    buf = String(ID);
                break;
                case 2:
                    position = buf.toInt();
                    buf = String(position);
                break;
                case 3:
                    parmTimeUp = buf.toInt();
                    buf = String(parmTimeUp);
                break;
                case 4:
                    parmTimeDown = buf.toInt();
                    buf = String(parmTimeDown);
                break;
                case 5:
                    parmPinPowerOn = buf.toInt();
                    buf = String(parmPinPowerOn);
                break;
                case 6:
                    parmPinDirection = buf.toInt();
                    buf = String(parmPinDirection);
                break;
                case 7:
                    name = buf;            
                break;
                case 8:
                    group = buf;                 
                break;
                //9
                default:
                {
                    int len = buf.toInt();
                    if (len != fileLen)
                    {
                        SerialPrintln(">>> Error in file %s: %d != %d", GetFileName(ID).c_str(), len, fileLen);        
                        SerialPrintln(">>> Error in file %s: %d,%d,%d,%d,%d,%d,%s,%s", GetFileName(ID).c_str(), ID, position, parmTimeUp, parmTimeUp, parmPinPowerOn, parmPinDirection, name.c_str(), group.c_str());
                    }
                    //return (len == fileLen);
                    return (len > 0);
                }
                break;
            }
            fileLen += buf.length();
            step++;
        }
        outfile.close();
    }
    else
    {
        SerialPrintln(">>> Error reading a file %s", GetFileName(ID).c_str());        
    }

    return false;
}
/*
void ABlindData::Init(byte _ID, byte _pinPowerOn, byte _pinDirection, unsigned long _timeUp, unsigned long _timeDown)
{
    position            = 0;
    ID                  = _ID;
    parmPinPowerOn      = _pinPowerOn;
    parmPinDirection    = _pinDirection;
    parmTimeUp          = _timeUp; //milisekundy
    parmTimeDown        = _timeDown;
}
*/
void ABlindData::Init(ABlindData& _data)
{
    ID                  = _data.ID;
    parmPinPowerOn      = _data.parmPinPowerOn;
    parmPinDirection    = _data.parmPinDirection;
    parmTimeUp          = _data.parmTimeUp; //milisekundy
    parmTimeDown        = _data.parmTimeDown;
    position            = _data.position;

    name = _data.name;
    group = _data.group;
}