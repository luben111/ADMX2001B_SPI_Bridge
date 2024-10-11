//================================================================
// ADMX2001B USB to SPI bridge
// SPI Command definitions
// IDEX Biometrics UK
// Written by Luben Hristov
//
// 09-08-24 -- Creating the file with command definitions. Was used Analog Device code as base
//
//================================================================
#ifndef _SPI_COMMANDS_H
#define _SPI_COMMANDS_H

#define CMD_STATUS_READ        (0x00) /** Read status */
#define CMD_RESULT_READ        (0x01) /** Read result */
#define CMD_CLEAR_ERROR        (0x02) /** Clear error */
#define CMD_FIFO_READ          (0x03) /** Read fifo */
#define CMD_CALIBRATE          (0x04) /** Command to calibrate */
#define CMD_COMPENSATION       (0x05) /** Command to read compensation value */
#define CMD_CAL_READ           (0x06) /** Command to read calibration coefficient */
#define CMD_STORE_CAL          (0x08) /** Command to store calibration coefficient */
#define CMD_RESET_CAL          (0x09) /** Command to store calibration coefficient */
#define CMD_ERR_CHECK          (0x0A) /** Command to switch on or off error check */
#define CMD_SELF_TEST          (0x0B) /** Command to switch on or off error check */
#define CMD_COMP_READ          (0x0C) /** Command to read calibration coefficient */
#define CMD_STORE_COMP         (0x0D) /** Command to store calibration coefficient */
#define CMD_TEMPERATURE        (0x0E) /** Command to read temperature */
#define CMD_Z                  (0X0F) /** Command Z */
#define CMD_ERASE_CALIBRATION  (0X10) /** Erase all calibaration data in FLASH */
#define CMD_READ_SWEEP_POINTS  (0X11) /** Read sweep points */
#define CMD_RESET              (0X12) /** Reset */
#define CMD_CURRENT_GAIN_TABLE (0X13) /** Current gain table */
#define CMD_VOLTAGE_GAIN_TABLE (0X14) /** Voltage gain table */
#define CMD_FW_VERSION         (0X15) /** Version Details */
#define CMD_IP_REGISTERS       (0X16) /** Command to read or write spi regsiters */
#define CMD_INITIATE           (0X17) /** Initiates the measurement */
#define CMD_TRIGGER            (0X18) /** Triggers the measurement */
#define CMD_FETCH              (0X19) /** Fetch the last measurement */
#define CMD_ABORT              (0X1A) /** Aborts the measurement */
#define CMD_CAL_COMMIT         (0X1B) /** Calibration commit command */
#define CMD_SET_PASSWORD       (0X1C) /** Calibration commit password set */
#define CMD_GET_PASSWORD       (0X1D) /** Calibration commit get password */
#define CMD_IP_REGISTERS_U32   (0X1E) /** Command to read or write spi regsiters with address than uint16 */
#define CMD_FREQUENCY          (0X23) /** lcr frequecny */
#define CMD_INTEGRATION        (0X24) /** lcr integration time */
#define CMD_MAGNITUDE          (0X25) /** dds magnitude */
#define CMD_OFFSET             (0X26) /** dds offset */
#define CMD_DDS_GAIN           (0X27) /** dds gain */
#define CMD_VOLTAGE_GAIN       (0X28) /** lcr voltage gain */
#define CMD_CURRENT_GAIN       (0X29) /** lcr current gain */
#define CMD_AVERAGE            (0X2A) /** lcr average */
#define CMD_MDELAY             (0X2B) /** lcr measurement delay */
#define CMD_TDELAY             (0X2C) /** lcr trigger delay */
#define CMD_TCOUNT             (0X2D) /** lcr trigger count */
#define CMD_SWEEP_START        (0X30) /** Sweep Start */
#define CMD_SWEEP_END          (0X31) /** Sweep End*/
#define CMD_SWEEP_TYPE         (0X32) /** Sweep Type */
#define CMD_SWEEP_SCALE        (0X33) /** lcr sweep scale log or linear */
#define CMD_CYCLES             (0X34) /** lcr cycles */
#define CMD_CELSIUS            (0X3B) /** lcr celsius */
#define CMD_RADIANS            (0X3C) /** lcr radians */
#define CMD_LOCK               (0X3D) /** lcr lock */
#define CMD_DISPLAY            (0X41) /** lcr display */
#define CMD_COUNT              (0X42) /** lcr sample count */
#define CMD_CORRECTION_MODE    (0X43) /** lcr correction mode */
#define CMD_OVERFLOW_CHECK     (0X44) /** Check ADC and FIFO overflow */
#define CMD_RESISTANCE_MODE    (0X45) /** DC resistance mode */
#define CMD_ENABLE_AUTORANGE   (0X46) /** Enable Auto range gain for ADC channels */
#define CMD_SAMPLE_CLOCKS      (0X47) /** Minimum sample clocks */
#define CMD_ECHO_MODE          (0X48) /** Check echo mode is enabled or disabled */
#define CMD_BOARD_REV          (0XC9) /** Reads the boards revision */
#define CMD_TRIGGER_MODE       (0X4A) /** Trigger mode */
#define CMD_STATE              (0XCD) /** State of behaviour model - only read allowed */
#define CMD_ACTUAL_COUNT       (0X4F) /** Actual measurement count value */
#define CMD_RUN_SELF_TEST      (0x51) /** run internal selftest */
#define CMD_SET_GPIO           (0x56) /** set the status of different GPIO 0..7 pins on ADMX2001 board*/

#define CMD_SELF_TEST_STATUS   (0xD1) /** Check last selftest result */
#define CMD_UNIQUE_ID          (0xD2) /** Unique ID - addr0 - lower 32 , addr 1 - higher 32 */
#define CMD_WARNING_READ       (0xD3) /** Read Warning Message Type */

#define ADDRESS_SHORT_CAL         1  // short calibration runs on  address 1
#define ADDRESS_OPEN_CAL          2  // short calibration runs on  address 1
#define ADDRESS_LOAD_CAL          3  // short calibration runs on  address 1
#define ADDRESS_LOAD_CAL_SET_RT   4  // set RT for load calibration
#define ADDRESS_LOAD_CAL_SET_XT   5  // set XT for load calibration
#define ADDRESS_RELOAD_CAL      0xFF // reload calibration

#define CMND_READ_MASK         (0x80) /** Read bit - commands will become read */
#define CMD_NOT_DEFINED        (0xFF) /** Not defined or unsupported command */

#define MASK_RESET_ALL_CAL    0xFF   // this will reset all calibrations in memory
#define ADDRESS_TIMESTAMP     12   // this is the address where we add the timestamp during <calibrate commit>
#define ADDRESS_CAL_COMMIT    0xFF   // this is the address to trigger the <calibrate commit> - compare password and store data
#define ADDRESS_CAL_ERASE     0xFF   // this is the address to trigger the <calibrate commit> - compare password and store data

#define MASK_CAL_CORRECTION    0x00000003 // check if the correction is ON/OFF (including comp)

#define ADMX200X_STATUS_MEASURE_DONE_BITM 0x80000000 /** Bit mask for measure done bit */
#define ADMX200X_STATUS_DONE_BITM         0x40000000 /** Bit mask for done field bits */
#define ADMX200X_STATUS_ERROR_BITM        0x20000000 /** Bit mask for status field bits */
#define ADMX200X_STATUS_WARN_BITM         0x10000000 /** Bit mask for warning field bits */   
#define ADMX200X_STATUS_FIFO_ERROR_BITM   0x08000000 /** Bit mask for fifo error */    
#define ADMX200X_STATUS_FIFO_DEPTH_BITM   0x03FF0000 /** Bit mask for fifo depth field bits */  
#define ADMX200X_STATUS_CODE_BITM         0x0000FFFF /** Bit mask for command result code field bits */ 
   
#define ADMX_STATUS_SUCCESS                 0x0 /** Success */
#define ADMX_STATUS_FAILED                  0x1 /** Failed */
#define ADMX_STATUS_TIMEOUT                 0x2 /** Timeout */
#define ADMX_STATUS_INVALID_ATTRIBUTE       0x3 /** Invalid attribute */
#define ADMX_STATUS_ATTR_OUT_OF_RANGE       0x4 /** Attribute value out of range */
#define ADMX_STATUS_INVALID_ADDRESS         0x5 /** Invalid address of command */
#define ADMX_STATUS_UNCOMMITED_CAL          0x6 /** Uncommitted calibration coeffs */
#define ADMX_STATUS_INVALID_CURRENT_GAIN    0x7 /** Invalid volt/current gain*/
#define ADMX_STATUS_INVALID_DISPLAY_MODE    0x8 /** Invalid display mode for DC res mode*/
#define ADMX_STATUS_INVALID_SWEEP_TYPE      0x9 /** Invalid sweep type for DC mode */
#define ADMX_STATUS_INVALID_SWEEP_RANGE     0xA /** Invalid sweep range */
#define ADMX_STATUS_INVALID_CAL_COEFF_TYPE  0xB /** Invalid AC calibration coefficient type */
#define ADMX_STATUS_TRIGGER_OVERFLOW        0xC /** System is not ready to take trigger */
#define ADMX_STATUS_INVALID_CAL_TYPE        0xD /** Invalid calibration type */
#define ADMX_STATUS_INVALID_GAIN            0xE /** Invalid DC calibration coefficient type */
#define ADMX_STATUS_COMP_FAILED             0xF /** Calibration or compensation failed */

#define ADMX_STATUS_INVALID_COMMAND_STATE  0x10 /** Invalid command for the state */
#define ADMX_STATUS_LOG_ZERO_ERROR         0x20 /** Log of zero error */
#define ADMX_STATUS_LOG_SIGN_ERROR         0x40 /** Sign change for log error */
#define ADMX_STATUS_VOLT_ADC_ERROR         0x80 /** Voltage ADC saturated error */
#define ADMX_STATUS_CURR_ADC_ERROR        0x100 /** Current ADC saturated error */
#define ADMX_STATUS_FIFO_ERROR            0x200 /** FIFO over/under flow error */
#define ADMX_STATUS_COUNT_EXCEEDED        0x400 /** Sweep count maximum value exceeded */

#define MASK_ALL_ERROR_MSG                0x7FF // all error messages fit into this


//========== WARNING DEFINITIONS
#define DDS_NCO_FREQ_WARN          1   /** DDS & NCO Frequency are not equal warning */
#define CAL_LOAD_FAIL_WARN         2   /** Calibration failed warning */
#define AUTORANGE_DISABLE_WARN     4   /** Autorange disabled warning */
#define AUTORANGE_FAIL_WARN        8   /** Autorange failed warning */
#define SWEEPCOUNT_WARN           16   /** Sweep count warning */
#define MAG_EXCEED_WARN           32   /** Magnitude limit warning */
#define OFFSET_LIMITED_WARN       64   /** Offset limit warning */
#define OFFSET_POS_EXCEED_WARN   128   /** Positive offset exceed warning */
#define OFFSET_NEG_EXCEED_WARN   256   /** Negative offset exceed warning */

#define MASK_ALL_WARNING_MSG     0x1FF  // all warning messages fit into this

//--------- CALIBRATION COMMANDS FOR READING THE COEFFICIENTS & DATA WITH CMD_CAL_READ (0x06) OR CMD_COMP_READ (0x0C)
#define CALL_ADDR_Ro             0b00000  // address of Ro
#define CALL_ADDR_Xo             0b00010  // address of Xo
#define CALL_ADDR_Go             0b00100  // address of Go
#define CALL_ADDR_Bo             0b00110  // address of Bo
                                        
#define CALL_ADDR_Rs             0b01000  // address of Rs
#define CALL_ADDR_Xs             0b01010  // address of Xs
#define CALL_ADDR_Gs             0b01100  // address of Gs
#define CALL_ADDR_Bs             0b01110  // address of Bs
                                        
#define CALL_ADDR_Rg             0b10000  // address of Rg
#define CALL_ADDR_Xg             0b10010  // address of Xg
#define CALL_ADDR_Gg             0b10100  // address of Gg
#define CALL_ADDR_Bg             0b10110  // address of Bg

#define CALL_ADDR_AC_STATUS      0b00001  // address of AC calibration status
#define CALL_ADDR_AC_FREQ        0b00011  // address of calibration frequency
#define CALL_ADDR_AC_TEMP        0b00101  // address of calibration temperature

// ----------- the commands below are NOT applicable for CMD_COMP_READ (0x0C) - use them only for CMD_CAL_READ (0x06)
#define CALL_ADDR_Rdo            0b11000  // address of Rdo
#define CALL_ADDR_Rdg            0b11010  // address of Rdg
#define CALL_ADDR_DC_STATUS      0b00111  // address of DC calibration status
#define CALL_ADDR_DC_FREQ        0b01001  // address of DC calibration frequency
#define CALL_ADDR_DC_TEMP        0b01011  // address of DC calibration temperature

#define MASK_LSB_COEFFICIENT     0x0000   // mask for LSB data (it's zero, but we need to indicate this bit status)
#define MASK_MSB_COEFFICIENT     0x4000   // mask for LSB data
#define SHIFT_ADDR_READ_CAL              9   // shift the address masks to get in place for readcal

#define MASK_SHORT_DONE          0x00F    // check for completed SHORT mask in 0x06 / calibration status address 0b00001
#define MASK_OPEN_DONE           0x0F0    // check for completed OPEN mask in 0x06 / calibration status address 0b00001
#define MASK_LOAD_DONE           0xF00    // check for completed LOAD mask in 0x06 / calibration status address 0b00001

#define SHIFT_ADDR_STORE_CAL             10   // shift the address masks to get in place for storecal


#endif /* end _SPI_COMMANDS_H */