/************************************************************************
 *
 * Title:             Debug_interface()
 *
 * Purpose:           Send bytes of data to an interface card
 *                    connected to a PC. The interface box
 *                    supports special SPI to USB protocol.
 *
 ************************************************************************/
#ifndef _LIF_H
#define _LIF_H
#include <Arduino.h>     // when we use definitions like byte, etc. we need to include it

#define LIF_CLK  		D0	// clock of the debugger
#define LIF_DATA 		D1	// data of the debugger

void send_debug_int8(byte datas);   // prototype LIF interface
void send_debug_int16(uint16_t signal16); // send temp16 to LIF
void send_debug_int32(uint32_t signal32); // send temp32 to LIF
void Pulse_LIF_CLK(void); // mke short pulse on LIF CLK pin
void Pulse_LIF_DATA(void); // mke short pulse on LIF DATA pin

#endif // end _LIF_H