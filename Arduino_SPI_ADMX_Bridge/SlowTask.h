//================================================================
// ADMX2001B USB to SPI bridge for Arduino UNO R4 Minima
// Executing tasks made on slower pace (like wait for DONE)
// IDEX Biometrics UK
// Written by Luben Hristov
//
// Arduino UNO R4 Minima for attachInterrupt() useable pins are: 2, 3 like:  attachInterrupt(digitalPinToInterrupt(pin_size_t pin)....)
// 07-08-24 -- Starting the project, testing the serial, setting DTR to receive data
//
//================================================================

#ifndef _SLOW_TASK_H
#define _SLOW_TASK_H

#include <Arduino.h>       // when we use definitions like byte, etc. we need to include it
#include <stdio.h>         // print commands and other stuff
#include "CmndProcess.h"   // inlcude main functionality
#include "Arduino_SPI_ADMX_Bridge.h"    // load file with all definitions (like int myVar;) and declarations (extern int myVar;) 


//--------- Function prototypes -----------------------------------------------------------
void ExecuteSlowTask(void);   // here we execute commands on regular intervals like wait for DONE and wait for MEASURE_DONE

//--------- External variables -----------------------------------------------------------
extern int measureZ_counter;            // keeps track of the sequential samples (when count > 1)
extern stateMeasureZ_t stateMeasureZ;   // this is the state machine for measure

extern int  depthFIFO;                  // how many record reside in the FIFO
extern bool flag_MEASURE_DONE ;         // set flag measure done
extern bool flag_DONE         ;         // set flag done
extern bool flag_ERROR        ;         // do we have error or not
extern int pendingRec;
extern int inQueue;


//---------- DEFINITIONS -----------------------------------------------------------------
#define LARGEST_UNIGNED_LONG 4294967295  // this is the largest unsigned long number, we used it for compensation of rollower in millis()
#define FLOAT_PRECISION   7              // how many digits floating point precision to output (7 in CLI)

#endif // end  _SLOW_TASK_H