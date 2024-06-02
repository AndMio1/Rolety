/* 
*  Name:	  ArduinoMegaBlinds.ino
*  Created:   09.01.2023
*  Author:	  Andrzej Miozga
*/
#ifndef file_blind_h
#define file_blind_h

#include <limits.h>
enum BlindState {noInit, moveUp, moveDown, stopedUp, stopedDown, Middle};//stan Unknown jest po włączeniu zasilania

/*
1. Przekażnik włącza się stanem LOW, konstruktow ustawia stany HIGH dla dwóch przekażników
2. Domyślny stan wyjść adruiono to high
3. Przekażnik zasilania, stan HIGH - wyłączony, zasilanie odłączone, stan LOW włącza i zasilanie kierowana na przekażnik kierunku
   Przekażnik kierunku,  stan HIGH - kierunke GÓRA, satn LOW wącza cewkę, kierunke DÓŁ
*/
class ABlindData
{
    public:
    static const byte OpenedPos = 100;   //percentage postition - top  - BLIND_OPEN (0)
    static const byte ClosedPos = 0;     //percentage postition - down - BLIND_CLOSED (100)

public:
    unsigned int       parmTimeUp;         //czas podnoszenia, wartośc stała [msek] 
    unsigned int       parmTimeDown;       //czas opuszczania, wartośc stała [msek]
    byte               parmPinPowerOn;     //pin dla przekaznika sterujacego zasilaniem
    byte               parmPinDirection;   //pnd dla przekażnika kierunku 0-dół, 1-góra (domyślny)
    byte               ID;
    byte               position;            //aktualny procent otwarcia 0-otwarte, 100-zamkniete
    String             name;
    String             group;

private:
    String             GetFileName(byte _id);

public:
    void Init(ABlindData& _data);

    void writeCfg();
    bool readCfg();    
};

bool sendMyMessage(byte _id, byte _position, bool _stop);

void SerialPrintln(const char* input, ...);

class ABlind
{
    private:
        void               SetDirection(boolean _downUp);  //włącza przekażnik kierunku rolety
        void               SetPower(boolean     _offOn);   //włącza przekażnik zasilania 

        BlindState         state;           //stan obecny 
        int8_t             offset;          //postep podczas ruchu wyliczany z expectedTime
        unsigned long      started;         //czas załączenia, uruchomienia silnika
        unsigned long      expectedTime;    //czas pozostały do wyłączenia, dekrementowany przy każdej pętli
        boolean            sendUpdate;
        ABlindData         data;
        bool               selected;

    protected:

        byte            validatePosition(int8_t _percent);
        unsigned long   elapsedTime();        

    public: 
        ABlind();

        void BlindSetup(byte _ID, byte _pinPowerOn, byte _pinDirection, unsigned long _timeUp, unsigned long _timeDown, byte _position, const char* _group = "");

        void setStop(bool _powerOff = true);     //natychmiastowe zatrzymanie
        void setClose();    //procent 100, na ile ustawić a nie o ile przestawić
        void setOpen();     //procent 0, na ile ustawić a nie o ile przestawić
        void setPosition(byte _percent); //procent 0-100, 100 to zamkniete
        
        byte doLoop(); //wywoływać w pętli głównej, aby kontrolować ruch/zatrzymanie

        //void doInit(); //wymuszenie zamknięcie rolety do końca

        bool sendMessage();

        byte        Position();
        BlindState  State();
        const char* Name();
        const char* Group();
        bool        isGroup(String& _group);
        bool        isSelected();
        void        setSelected(bool _selected);
};


#endif