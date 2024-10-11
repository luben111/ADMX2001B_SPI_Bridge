//================================================================
// ADMX2001B USB to SPI bridge
// ANSI terminal commands
// IDEX Biometrics UK
// Written by Luben Hristov
//
// 12-08-24 -- Creating the file with ANSI command definitions
//
//================================================================
#ifndef _ANSI_COMMANDS_H
#define _ANSI_COMMANDS_H

const char MEAS_Z0[]              = "z";                // command for measuring inpedance
const char TEMPERAT0[]            = "temperature";      // command for reading the temperature
const char    TEMP_CELSIUS[]      = "cls";              // temperature in celsius
#define TEMP_CELSIUS_VAL    1                           // the Celsius enum value
const char    TEMP_FARENHEIT[]    = "fht";              // temperature in Farenheit
#define TEMP_FARENHEIT_VAL  0                           // the Farenheit enum value

const char ABORT0[]              = "abort";             // command for aborting the measurements

const char FREQUENCY0[]           = "frequency";        // command for setting the frequency in kHz
const char FREQUENCY_POS_STR[]    = "kHz";              // post string of frequency command

const char MAGNITUDE0[]           = "magnitude";        // command for setting the magnitude in V
const char MAGNITUDE_POS_STR[]    = "V";                // post string of magnitude command

const char OFFSET0[]              = "offset";           // command for setting the offset in V
const char OFFSET_POS_STR[]       = "V";                // post string of offset command

const char SETGAIN0[]             = "setgain";          // command for setting vgain or igain
const char    SETGAIN_AUTO1[]     = "auto";             // sub1 for auto mode
const char    SETGAIN_VGAIN1[]    = "ch0";              // sub1 for setting vgain (ch0)
const char    SETGAIN_IGAIN1[]    = "ch1";              // sub1 for setting vgain (ch0)
const char    SETGAIN_RESP_V1[]   = "volt gain = ";     // set voltage gain
const char    SETGAIN_RESP_I1[]   = "curr gain = ";     // set current gain

const char TRIG_MODE0[]           = "trig_mode";         // set trigger mode
const char    TRIG_MODE_INT[]     = "internal";         // internal trigger mode
#define TRIG_MODE_INTER_VAL    0                        // the internal trigger mode value
const char    TRIG_MODE_EXT[]     = "external";         // external trigger modet
#define TRIG_MODE_EXTERN_VAL   1                        // the external trigger mode value

const char AVERAGE0[]             = "average";          // command for setting the averaging per sample
const char DISPLAY0[]             = "display";          // command for setting the display mode

const char MDELAY0[]               = "mdelay";          // command for setting the setting the measuring delay
const char MDELAY_POS_STR[]        = "msec";            // post string of mdelay command

const char TDELAY0[]               = "tdelay";          // command for setting the setting the trigger delay
const char TDELAY_POS_STR[]        = "msec";            // post string of tdelay command

const char COUNT0[]                = "count";           // command for setting the number of counts per measurement
const char TCOUNT0[]               = "tcount";          // command for setting the number of trigger counts

const char SWEEPTYPE0[]            = "sweep_type";      // command for setting the sweep type
const char    SWEEPTYPE_FREQ1[]    = "frequency";       // sub1 for type as frequency
const char    SWEEPTYPE_MAGN1[]    = "magnitude";       // sub1 for type as magnitude
const char    SWEEPTYPE_OFFSET1[]  = "offset";          // sub1 for type as offset
const char    SWEEPTYPE_OFF1[]     = "off";             // sub1 for sweep off

const char SWEEP_SCALE0[]          = "sweep_scale";     // type fo the sweep scale
const char    SWEEP_SCALE_LIN1[]   = "linear";          // internal trigger mode
#define SWEEP_SCALE_LIN_VAL    0                        // the internal trigger mode value
const char    SWEEP_SCALE_LOG1[]   = "log";             // external trigger modet
#define SWEEP_SCALE_LOG_VAL    1                        // the external trigger mode value

const char CALIBRATE0[]            = "calibrate";      // command for calibration
const char    CALIBRATE_OPEN1[]    = "open";           // sub1 start open calibration for particular freq/vgain/igain trinity
const char    CALIBRATE_SHORT1[]   = "short";          // sub1 start short calibration for particular freq/vgain/igain trinity
const char    CALIBRATE_RT_XT1[]   = "rt";             // sub1 start load calibration - sub2 is the value of RT
const char      CALIBRATE_XT_XT3[] = "xt";             // sub3 start load calibration - sub4 is the value of XT

const char    CALIBRATE_ON1[]      = "on";             // sub1 set calibration to on (apply calibration for all measurements)
const char    CALIBRATE_OFF1[]     = "off";            // sub1 set calibration to off (calibration not applied to measured data)
const char    CALIBRATE_RELOAD1[]  = "reload";         // sub1 to reload calibration
const char    CALIBRATE_COMMIT1[]  = "commit";         // sub1 to comnit calibration (store in FLASH coeff for curent freq/vgain/igain trinity)
const char    CALIBRATE_ERASE1[]   = "erase";          // sub1 to erase calibration from FLASH (will require to set password)
const char    CALIBRATE_LIST1[]    = "list";           // sub1 to list the availible calibration (withot sub2 shows the frequencies, with sub2=freq for particular ferquency)

const char READCAL0[]              = "rdcal";          // read from RAM the calibration coefficients for the assigned freq/vgain/igain trinity
const char RESETCAL0[]             = "resetcal";       // resets in RAM calibration coefficients for the assigned freq/vgain/igain trinity
const char STORECAL0[]             = "storecal";       // store specified calibration coefficient (Ro,Xo,Go,Bo...) in RAM for the assigned freq/vgain/igain trinity

const char    STORE_CAL_FIELDS1[12][3] = {"Ro","Xo","Go","Bo",  "Rs","Xs","Gs","Bs",  "Rg","Xg","Gg","Bg"};  // array of tokens

const char IDN0[]                  = "*idn?";          // read the module ID and firmware
const char RESET0[]                = "reset";          // resets the module
const char SELFTEST0[]             = "selftest";       // runs selftest
const char GPIO_CTRL0[]            = "gpio_ctrl";      // GPIO pins control

const char CMND_VOID[]             = "void";           // void command, for debugging purposes


//------ Some specific strings for SPI bridge
#define BRIDGE_RESET              (char(0xB0))          // this single char command (0xB0) will reset the bridge instantly to init state
const char VOID_STR[]              = "";                // command for measuring inpedance

#endif /* end _ANSI_COMMANDS_H */