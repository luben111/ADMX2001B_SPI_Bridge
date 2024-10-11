//================================================================
// ADMX2001B USB to SPI bridge
// Function calls and constants
// IDEX Biometrics UK
// Written by Luben Hristov
//
// 09-08-24 -- Creating the file, moving constants into this file
//
//================================================================
#ifndef _SPI_ADMX_BRIDGE_H
#define _SPI_ADMX_BRIDGE_H

#include <Arduino.h>      // when we use definitions like byte, etc. we need to include it
#include "SPI_cmnd.h" // get info about the SPI command codes


//-------- Macros and other definitions
#define getAdmxStatus()   Single_ADMX_Frame(CMD_STATUS_READ,0,0)   // poll the status command 
#define disapleISR  noInterrupts()       // disable interrupts - it's needed to poll the data
#define enableISR   interrupts()         // enable interrupts - needed for USB to work properly 

#define SIZE_RECEIVER_QUEUE    4000      // we need deeper queue for inpQueue to store the sent from PC commands
#define SIZE_RECORD_LEN_QUEUE  200      // we need shalower queue for storing the length of records

#endif  // end _SPI_ADMX_BRIDGE_H
