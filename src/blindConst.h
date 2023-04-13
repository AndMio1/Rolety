#ifndef file_blindconst_h
#define file_blindconst_h

#pragma warning(push)
#pragma warning(disable: _VCRUNTIME_DISABLED_WARNINGS)
/////////////////////////////////////

#define LOOP_DELAY   100  //opóznienie w pętli
#define RELAY_DELAY  100  //opóznienie właczena przekażnika
#define BLIND_OPEN     0  //percentage postition - top
#define BLIND_CLOSED 100  //percentage postition - down = closed

#define NUMBER_OF_BLINDS 5	// Total number of attached relays
#define RELAY_ON 1			// GPIO value to write to turn on attached relay
#define RELAY_OFF 0			// GPIO value to write to turn off attached relay
#define TIMEFACTOR 1000

#define PowerPinFirst  22
#define DirecPinFirst  23

//#pragma region MySensorsDefs
    // Enable debug prints to serial monitor
//#define MY_DEBUG
// Enable serial gateway
//#define MY_GATEWAY_SERIAL
// Enable gateway ethernet module type
#define MY_GATEWAY_W5100
// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
//#define MY_IP_ADDRESS 10,10,20,122
//#define MY_IP_GATEWAY_ADDRESS 10,10,20,1
//#define MY_IP_SUBNET_ADDRESS 255,255,255,0
//#define MY_PORT 5003
// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE
// Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_DURATION 60
#define MY_REPEATER_FEATURE
//#pragma endregion MySensorsDefs
////////////////////////////

#pragma warning(pop) // _VCRUNTIME_DISABLED_WARNINGS

#endif
