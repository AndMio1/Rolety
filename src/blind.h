#ifndef file_blind_h
#define file_blind_h

#include "Arduino.h"

enum BlindState {noInit, moveUp, moveDown, stopedUp, stopedDown, Middle};
//stan Unknown jest po włączeniu zasilania

/*
1. Przekażnik włącza się stanem LOW, konstruktow ustawia stany HIGH dla dwóch przekażników
2. Domyślny stan wyjść adruiono to high
3. Przekażnik zasilania, stan HIGH - wyłączony, zasilanie odłączone, stan LOW włącza i zasilanie kierowana na przekażnik kierunku
   Przekażnik kierunku,  stan HIGH - kierunke GÓRA, satn LOW wącza cewkę, kierunke DÓŁ
*/
struct ABlintPack
{
    unsigned int       parmTimeUp;      //czas podnoszenia, wartośc stała [msek] 
    unsigned int       parmTimeDown;    //czas opuszczania, wartośc stała [msek]

    BlindState         state;           //stan obecny 
    byte               position;        //aktualny procent otwarcia 0-otwarte, 100-zamkniete
};

void sendMyMessage(byte _id, byte _position);
void SerialPrintln(const char* input, ...);

class ABlind
{
    private:
        unsigned int       parmTimeUp;         //czas podnoszenia, wartośc stała [msek] 
        unsigned int       parmTimeDown;       //czas opuszczania, wartośc stała [msek]
        byte               parmPinPowerOn;     //pin dla przekaznika sterujacego zasilaniem
        byte               parmPinDirection;   //pnd dla przekażnika kierunku 0-dół, 1-góra (domyślny)
        byte               ID;

        void               SetDirection(boolean _downUp);  //włącza przekażnik kierunku rolety
        void               SetPower(boolean     _offOn);   //włącza przekażnik zasilania 

    protected:
        BlindState         state;       //stan obecny 
        byte               position;    //ostatnia pozycja / procent otwarcia 0-otwarte, 100-zamkniete
        int8_t             offset;      //postep podczas ruchu wyliczany z expectedTime

        unsigned long      started;     //czas załączenia, uruchomienia silnika
        unsigned long      expectedTime;  //czas pozostały do wyłączenia, dekrementowany przy każdej pętli
        boolean            sendUpdate;

    protected:
        byte validatePosition(int8_t _percent);
        unsigned long  elapsedTime();        

    public: 
        ABlind();

        void BlindSetup(byte _ID, byte _pinPowerOn, byte _pinDirection, unsigned long _timeUp /*sek*/, unsigned long _timeDown/*sek*/);

        ABlintPack pack();
        void unpack(ABlintPack _bpack);

        void setStop(bool _powerOff = true);     //natychmiastowe zatrzymanie
        void setClose();    //procent 100, na ile ustawić a nie o ile przestawić
        void setOpen();     //procent 0, na ile ustawić a nie o ile przestawić
        void setPosition(byte _percent); //procent 0-100, 100 to zamkniete
        
        byte doLoop(); //wywoływać w pętli głównej, aby kontrolować ruch/zatrzymanie

        void doInit(); //wymuszenie zamknięcie rolety do końca

        String getStateStr();
        void sendMessage();

        byte Position();
        BlindState  State();
};


#endif