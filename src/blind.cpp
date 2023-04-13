#include "blindConst.h"
#include "blind.h"
#include "limits.h"

ABlind::ABlind()
{
    position = 0;
    offset = 0;
    started          = 0;
    parmPinPowerOn   = 0;
    parmPinDirection = 0;
    parmTimeUp       = 0; //milisekundy
    parmTimeDown     = 0;

    state = BlindState::noInit;

    //pinMode(parmPinPowerOn, OUTPUT);
    //digitalWrite(parmPinDirection, HIGH);   // kierunek góra - stan domyślny wyjść    

    //pinMode(parmPinDirection, OUTPUT);
    //digitalWrite(parmPinPowerOn, HIGH);   //wyłącz sterowanie / zasilanie, stan domyślny wyjść    
}

void ABlind::BlindSetup(byte _ID, byte _pinPowerOn, byte _pinDirection, unsigned long _timeUp, unsigned long _timeDown)
{
    position    = 0;
    offset      = 0;
    ID                  = _ID;
    started             = 0;
    parmPinPowerOn      = _pinPowerOn;
    parmPinDirection    = _pinDirection;
    parmTimeUp          = (_timeUp   * TIMEFACTOR); //milisekundy
    parmTimeDown        = (_timeDown * TIMEFACTOR);
    sendUpdate          = true;

    state = BlindState::stopedUp;

    pinMode(parmPinPowerOn,   OUTPUT);
    pinMode(parmPinDirection, OUTPUT);

    digitalWrite(parmPinPowerOn,   HIGH);   //wyłącz sterowanie / zasilanie, stan domyślny wyjść    
    digitalWrite(parmPinDirection, HIGH);   //kierunek góra - stan domyślny wyjść    
}

ABlintPack ABlind::pack()
{
    ABlintPack bpack;

    return bpack;
}

void ABlind::unpack(ABlintPack _bpack)
{

}

void ABlind::SetDirection(boolean _downUp)
{
    if (_downUp == false)
        digitalWrite(parmPinDirection,  LOW);  // kierunek dół  - przekażnik załączony
    else
        digitalWrite(parmPinDirection,  HIGH); // kierunek góra - przekażnik wyłączony,stan domyślny wyjść            
    //delay(RELAY_DELAY);    
}

void ABlind::SetPower(boolean     _offOn)
{
    if (_offOn)
    {
        delay(RELAY_DELAY);    
        digitalWrite(parmPinPowerOn, LOW);      //włącz sterowanie
    }
    else
    {
        digitalWrite(parmPinPowerOn,    HIGH);  //wyłącz sterowanie - przekaźnik niezasilony
        delay(RELAY_DELAY);
        digitalWrite(parmPinDirection,  HIGH);  // kierunek góra - przekażnik wyłączony,stan domyślny wyjść
    }        
}

//podniesienie rolet w celu ustalenia pozycji początkowej
void ABlind::doInit()
{
    position = BLIND_OPEN;    
    setOpen();
}

byte ABlind::validatePosition(int8_t _percent)
{
    byte percent = (_percent % 100);
    if (_percent <= BLIND_OPEN)
        percent = BLIND_OPEN;
    else
        if (_percent >= BLIND_CLOSED)
            percent = BLIND_CLOSED;

    return percent;
}

void ABlind::setStop(bool _powerOff)
{
    unsigned long time = elapsedTime();
    int8_t        pos  = position+offset;

    if (_powerOff == true)
        SetPower(false);  //off

    if (started > 0 && expectedTime > 0)
    {   
        //calculate the current position
        if (state == BlindState::moveDown)
        {
            pos = (int8_t)(time * (unsigned long)100 / parmTimeDown);
            position = validatePosition((int8_t)(position + pos));
        }
        if (state == BlindState::moveUp)
        {
            pos = (int8_t)(time * (unsigned long)100 / parmTimeUp);
            position = validatePosition((int8_t)(position - pos));
        }
    }
    started      = 0;
    expectedTime = 0;
    offset       = 0;

    switch (position)
    {
        case BLIND_CLOSED:
            state = BlindState::stopedDown;
        break;

        case BLIND_OPEN:
            state = BlindState::stopedUp;
        break;

        default:
            state = BlindState::Middle;
    }
    sendUpdate = true;
}

void ABlind::setClose()    //procent 100, na ile ustawić a nie o ile przestawić
{
    SerialPrintln(">>> Blind close: %d(%ld)", ID, millis());
    setPosition(BLIND_CLOSED);
}

void ABlind::setOpen()     //procent 0, na ile ustawić a nie o ile przestawić
{
    SerialPrintln(">>> Blind open: %d (%ld)", ID, millis());
    setPosition(BLIND_OPEN);
}

void ABlind::setPosition(byte _percent) //procent 0-100, 100 to zamkniete
{
    byte newPos = validatePosition(_percent);
    BlindState  newState = BlindState::noInit;
    
    if ((position+offset) < newPos)
        newState  = BlindState::moveDown;
    else        
        if ((position+offset) > newPos)
            newState  = BlindState::moveUp;
    
    setStop(newState != state);

    if (position < newPos)
    {
        state  = BlindState::moveDown;
        newPos = newPos - position;
        
        expectedTime = (unsigned long)newPos * parmTimeDown / 100;
        SetDirection(false);//dół
        
        SerialPrintln("[%d] moveDown %d, %d, %d", ID, position, _percent, expectedTime);
    }
    else
        if (position > newPos)
        {
            state  = BlindState::moveUp;
            newPos = position - newPos;
            
            expectedTime = (unsigned long)newPos * parmTimeUp / 100;
            SetDirection(true);//góra

            SerialPrintln("[%d] moveUp %d, %d, %d", ID, position, _percent, expectedTime);            
        }

    if (state == BlindState::moveDown || state == BlindState::moveUp)
    {
        started = millis();
        SetPower(true);     //on
    }
    sendUpdate = true;
}

String ABlind::getStateStr()
{
    static char buffer[64];
    String  sstate = "?";

    switch(state)
    {
        case BlindState::noInit:
            sstate = "noInit"; break;
        case BlindState::moveUp:
            sstate = "moveUp"; break;
        case BlindState::moveDown:
            sstate = "moveDown"; break;
        case BlindState::stopedUp:
            sstate = "stopedUp"; break;
        case BlindState::stopedDown:
            sstate = "stopedDown"; break;
        case BlindState::Middle:
            sstate = "Middle"; break;
        default:
            sstate = "?";
    }

    sprintf(buffer, "ID: %hd, State: %s, Pos: %hd (%hd) (t: %lu)", ID, sstate.c_str(), position, offset, expectedTime);
    return buffer;
}

byte ABlind::doLoop()
{
    unsigned long time;
    int8_t newOffset = offset;

    if (state == BlindState::moveDown || state == BlindState::moveUp)
    {
        time = elapsedTime();
        
        if (time >= expectedTime)            
        {
            setStop();
        }
        else
        {
            //oblicz pozycję
            if (state == BlindState::moveDown)
            {   //oblicz jaki procent czsu upłynął
                offset = (int8_t)(100 * time / parmTimeDown);                
            }
            else
            {
                offset = -(int8_t)(100 * time / parmTimeUp);                
            }
        }
        if (newOffset != offset)
        {
            if (validatePosition(position+offset) == BLIND_OPEN || validatePosition(position+offset) == BLIND_CLOSED)
            {
                setStop();
            }
            sendUpdate = true;
        }
    }
    return (position+offset);
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

void ABlind::sendMessage()
{
    if (sendUpdate)
    {
        sendUpdate = false;
        sendMyMessage(ID, position+offset);        
        //SerialPrintln("[%d] upd %d, %d, %d", ID, position, offset, expectedTime);                    
    }
}

byte ABlind::Position()
{
    return validatePosition(position+offset);
}

BlindState ABlind::State()
{
    return state;
}