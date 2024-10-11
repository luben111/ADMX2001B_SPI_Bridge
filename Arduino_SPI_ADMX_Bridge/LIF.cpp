/************************************************************************
 *
 * Title:             Debug_interface()
 *
 * Purpose:           Send bytes of data to an interface card
 *                    connected to a PC. The interface box
 *                    supports special SPI to USB protocol.
 *
 * MAXIMAL CLOCK SPEED PER SINGLE BYTE - 1.5 MHz (MINIMUM 670 uS PER CLOCK)
 * MINIMAL DELAY BETWEEN BYTES - 25 uS
 * MINIMAL TIME GAP BETWEEN DATA BLOCKS - 1.5mS (AFTER 1.5mS WILL AUTORESET THE SEQUENCE AND WILL START RECORDING FROM BYTE 0)
 * INPUT BYTE - DATA SEQUENCE - MSBit first, LSBit last (the user should output first MS bit, then LS bits)
************************************************************************/



/************************************************************************
 *
 * Title:             send_debug_byte()
 *
 * Purpose:           Send out a byte of data via Luben's interface
 *
 *
 *             ,-----,-----,-----,-----,-----,-----,-----,-----,
 * DATA        | MSB |     |     |     |     |     |     | LSB |
 *       ------'-----'-----'-----'-----'-----'-----'-----'-----'-------
 *
 *               ,--,  ,--,  ,--,  ,--,  ,--,  ,--,  ,--,  ,--,
 * CLOCK         |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
 *      ---------'  '--'  '--'  '--'  '--'  '--'  '--'  '--'  '--------
 *
 ************************************************************************/
  
#include <Arduino.h>
#include "LIF.h"    // definitions and prototypes

//*************************************************************************
void send_debug_int8(byte datas) 	// procedure to output single to LIF box
//*************************************************************************
{
	
  for( byte ii = 8; ii != 0; ii--)
  {	
    int toWrite = (datas & 0x80)? 1: 0;		
    digitalWrite(LIF_DATA, toWrite);		// output data

    digitalWrite(LIF_CLK, HIGH);		// make a CLK pulse
    delayMicroseconds(1);
    digitalWrite(LIF_CLK, LOW);		  // output data		

    datas <<= 1; /* Shift next bit up, ready for output */
  }
	digitalWrite(LIF_DATA, LOW);	 // set data again in zero state (it's not neccessary)
  delayMicroseconds(25);
} // of of send byte

//************************************************************************
void send_debug_int16(uint16_t signal16) // send temp16 to LIF (MSB firt)
//************************************************************************
{
   send_debug_int8(signal16 >> 8);             // MSB last
   send_debug_int8(signal16 & 0xFF);           // LSB first
}

//************************************************************************
void send_debug_int32(uint32_t signal32) // send temp32 to LIF MSB first
//************************************************************************
{
   send_debug_int8((signal32 >> 24) & 0xFF);    // MSB last  
   send_debug_int8((signal32 >> 16) & 0xFF);      
   send_debug_int8((signal32 >> 8) & 0xFF);      
   send_debug_int8(signal32 & 0xFF);             // LSB first
}

//************************************************************************
void Pulse_LIF_CLK(void) // mke short pulse on LIF CLK pin
//************************************************************************
{
    digitalWrite(LIF_CLK, HIGH);		// make a CLK pulse
    digitalWrite(LIF_CLK, LOW);		  // output data		
}

//************************************************************************
void Pulse_LIF_DATA(void) // mke short pulse on LIF DATA pin
//************************************************************************
{
    digitalWrite(LIF_DATA, HIGH);		// make a CLK pulse
    digitalWrite(LIF_DATA, LOW);		  // output data		
}

