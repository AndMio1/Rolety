#ifndef file_blindconst_h
#define file_blindconst_h

/////////////////////////////////////
#define SHIELD_TYPE "W5x00 using Ethernet_Generic Library on SPI0/SPI"
#define BOARD_TYPE "AVR Mega"
//#define USE_THIS_SS_PIN 10
//#define ETH_USE_THIS_PIN 10
#define SD_USE_THIS_PIN  4 //SD card init
#define HTTP_SERVER_PORT 80
#define ETHERNET_LARGE_BUFFERS
/////////////////////////////////////
#define RELAY_DELAY      30  //opóznienie właczena przekażnika
#define NUMBER_OF_BLINDS 13	// Total number of attached relays
#define RELAY_ON   1			// GPIO value to write to turn on attached relay
#define RELAY_OFF  0			// GPIO value to write to turn off attached relay
#define TIMEFACTOR 1000

#define PowerPinFirst  22
#define DirecPinFirst  23
#define GroupAll  "/"

//MySensorsDefs
// Enable debug prints to serial monitor
//#define MY_DEBUG
// Enable gateway ethernet module type
#define MY_GATEWAY_W5100
#define MY_MAC_ADDRESS 0xDE,0xAD,0xBE,0xEF,0xFE,0xED
//#define MY_IP_ADDRESS 10,10,10,150
//#define MY_PORT 5003
#define MY_GATEWAY_FEATURE
//#define MY_CORE_ONLY

// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE
#define MY_INCLUSION_MODE_DURATION 60
////////////////////////////

//#pragma warning(pop) // _VCRUNTIME_DISABLED_WARNINGS
//#define _ETG_LOGLEVEL_                      3
//#define _ETHERNET_WEBSERVER_LOGLEVEL_       3

#endif
