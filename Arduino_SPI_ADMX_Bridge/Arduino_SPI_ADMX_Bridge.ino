//================================================================
// ADMX2001B USB to SPI bridge for Arduino UNO R4 Minima
// converts the ADMX ANSI commands into SPI sequences 
// IDEX Biometrics UK
// Written by Luben Hristov
//
// Arduino UNO R4 Minima for attachInterrupt() useable pins are: 2, 3 like:  attachInterrupt(digitalPinToInterrupt(pin_size_t pin)....)
// SPI bridge reset command is 0xB0 of >°< (very high priority) - you can copy the <degree> symbol and paste it into terminal to reset the pridge
// 07-08-24 -- Starting the project, testing the serial, setting DTR to receive data
// 02-09-24 -- Adding <calibrate commit>
// 30-09-24 -- Adding <gpio_ctrl> command 
//================================================================

#include <Strings.h>
//---------- CIRCULAR BUFFER SUPPORT -------------------------------------------------------------------
// Circular buffer is used to store the UART data into inpQueue (this is the largest queue - 2-3K)
// when CR/LF was detected the length of the block is stored into the second FIFO lenQueue, which is
// telling us how many records need to pull back for this particular command to poll the next command for preocessing
//#define CIRCULAR_BUFFER_INT_SAFE      // if we want to make the queues ISR safe
#include <CircularBuffer.hpp>           // adds Circular buffer
#include "Arduino_SPI_ADMX_Bridge.h"    // load file with all definitions (like int myVar;) and declarations (extern int myVar;) 
#include "CmndProcess.h"                // inlcude command processing definitions and prototypes
#include "SlowTask.h"                   // inlcude Slow task functionality
#include "ANSI_cmnd.h"                 // access definition of BRIDGE_RESET
#include "LIF.h"                        // include debugger interface (in this module we set the IO pins)

//-------- Global variables definitions used in the code
int stat_TX_LED = 1;              // control the TX LED status to toggle on transmissions 
int stat_RX_LED = 1;              // control the TX LED status to toggle on each received command
byte payload_SPI;                 // this variable is used for polling the SPI data
byte dat0, dat1, dat2, dat3, dummy;  // responses from SPI transmissions are stored here
uint32_t statusAdmx;              // here we store the status of the last operation
int curCommandLen = 0;            // static valiable for main asscociated with inpQueue, which keeps track how many records we 
int pendingRec = 0;               // keeps track of pending records
int inQueue = 0;                  // keeps track of the commands in the queue

CircularBuffer<char,    SIZE_RECEIVER_QUEUE>    inpQueue;     // define new queue - this is the input queue where all chars are accumulated
CircularBuffer<int16_t, SIZE_RECORD_LEN_QUEUE> recLenQueue;   // define new queue - this is the CRLF records length queue (also tell us how many commands are wauting in the queue)

//================================================================
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);   // the speed doesn't matter - it's USB channel and works on max speed possible
                          // this is must have in order to receive data, ensure that DTR and RTS are ON on receiver side!
  Serial._dtr = true;     // set the DTR signal to true to enable receiving the data (we can disable it by setting to FALSE)
                          // Serial.dtr(); // enable forever the reception - sets the private ignore_dtr=TRUE
                          // Not well documented feature is that Serial.dtr() enables the reception of the data  
  Serial.rts();
  Serial._rts = true;
 
  pinMode(SPI_SS_PIN, OUTPUT);      // set the SS pin as an output
  pinMode(SPI_MISO_PIN, OUTPUT);    // set the SPI_OUT pin as an output
  pinMode(SPI_MOSI_PIN, INPUT);     // set the SPI_IN as an output
  pinMode(SPI_CLK_PIN, OUTPUT);     // set the CLK as an output

  pinMode(LIF_DATA, OUTPUT);     // LIF data is output
  pinMode(LIF_CLK,  OUTPUT);     // LIF clock is output
  digitalWrite(LIF_DATA, LOW);   
  digitalWrite(LIF_CLK, LOW);    


  digitalWrite(SPI_SS_PIN, HIGH);   // Keeep SS in high state
  digitalWrite(SPI_CLK_PIN, LOW);   // CLK idle state is zero

  pinMode(LED_RX, OUTPUT);          // RX LED is output
  pinMode(LED_TX, OUTPUT);          // TX LED is output

  digitalWrite(LED_RX, HIGH);       // RX led off
  digitalWrite(LED_TX, HIGH);       // TX LED off

  InitialiseSPI();      // intialise the SPI communication

  inpQueue.clear();     // flush the queue
  recLenQueue.clear();  // flush the queue

}  // end of Setup section



//================================================================
// MAIN loop is here - the functionality of the program
//================================================================
void loop() {

//--------- Data receiving is here --------------
  while (Serial.available()) {           // if new data available - read all pending data
    
    char inChar = (char)Serial.read();   // get the new byte:

    if (inChar == BRIDGE_RESET) {        // high priority task to reset the bridge (char 0xB0 - degree)
      recLenQueue.clear();  // clears this FIFO
      inpQueue.clear();     // clears input pending data 
      curCommandLen = 0;    // void all data 
      stateMeasureZ = IDLE; // set status to IDLE and all peding measurements will be lost
      //------ Here we can pull down the hardware reset for the ADMX module and initialise it (of cut the power supply for short time)
      Bridge_SerialPrintLn("Bridge Reset");   // here we print the special character 0x0C which works as LabView delimiter for the commands
      Bridge_SerialPrintDelimiter();   // here we print the special character 0x0C which works as LabView delimiter for the commands
    } 
    else if (inChar == '\n') {           // if the incoming character is a LF newline, set a flag so the main loop can
      if (curCommandLen > 0) {           // check if we already stored some data
        recLenQueue.push(curCommandLen); // push the number of chars for this record 
        curCommandLen = 0;               // new start
        pendingRec = recLenQueue.size();  // how many recrds
        stat_RX_LED = (stat_RX_LED != LOW)? LOW : HIGH;  // toggle the state
        digitalWrite(LED_RX, stat_RX_LED);   // toggle the RX LED
      }
    }
    else if (inChar != '\r') {           // we throw away line CR characters, so if not CR we push char into FIFO
      inpQueue.push(inChar);             // push the data into the FIFO
      curCommandLen++;                   // one more pending char in the new sequence
    }

  } // data arrived on serial port

  inQueue = recLenQueue.size();
  
  if (stateMeasureZ == IDLE) // there are no active tasks going on - so we can try to poll a new command for execution
  {
    if (inQueue > 0)  // there are pending commands - let's process them, notice that we pull only one command to ensure we don't stuck here
    {
      pendingRec = recLenQueue.shift();  // get out one record

      for (int ii = 0; ii < pendingRec; ii++) {  // extract the next command from the circular FIFO
        commandStr[ii] = inpQueue.shift();
      }
      commandStr[pendingRec] = char(0); // add string end

      Bridge_SerialPrintLn(commandStr);       //echo - output the original string (if we need echo - uncomment this row)
      CommandSplitter(pendingRec);      // split the commands into up to 5 fields, we pass the length of the available records and get arguments into sub0..4

      Command_Processor();              // here we extract the commands and arguments and we send the data to SPI, in case we need to wait for some commands 
                                        // like z or claibrate - we use the secondary processing in SecondaryCommandPorcessor()
    } // there were some pending command lines - we process them one by one to avoid locking the MCU in this place for long
  }  // no active task 

  ExecuteSlowTask();   // here we execute commands on regular intervals like wait for DONE and wait for MEASURE_DONE


}  // end of the loop

