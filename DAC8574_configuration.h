/*
 * DAC8574_configuration.h
 *
 *  Created on: 29 may. 2023
 *      Author: psl
 *      H file which contains the configuration of the DAC
 */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include "../my_periph/my_periph.h"

#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>

/**************************************************************************
    DAC8574_configuation.h
    => Configure the digital to analog converter device by I2C protocol

**************************************************************************/
#ifndef SRC_DAC8574_CONFIGURATION_H_
#define SRC_DAC8574_CONFIGURATION_H_

/************************************************
    DAC8574 I2C ADDRESSES
*************************************************/

 	#define DAC8574_DEFAULT_ADDRESS         (0b10011) // 1001 1aa depending of the two last bits set
	#define WRITE_BUFF_DAC 3
	#define READ_BUFF_DAC 6

typedef uint8_t AddressType;
/************************************************
    DAC8574 I2C ADDRESSES - BROADCAST ADDRESS BYTE
*************************************************/
 	 #define DAC8574_BROADCAST_ADDRESS         (0x90)
/************************************************
    CONVERSION DELAY (in mS)
*************************************************/
    #define DAC8574_CONVERSIONDELAY         (1000) // => 1s
/************************************************
    CONTROL BYTE
    TABLE 1. CONTROL REGISTER BIT DESCRIPTIONS
*************************************************/

/*********************
    BIT 7 : A3
    BIT 6 : A2
    EXTENDED ADDRESS BIT
    State bits must match state  of pins A3 and A2
**********************/

/*********************
    BIT 5 : L1 (load 1 : Mode Select Bit)
    BIT 4 : L0 (load 0 : Mode Select Bit)
    USED for selecting the update mode
**********************/
	#define DAC8574_MS_MASK             (0x30)
	#define DAC8574_MS_STORE            (0x00) // STORE I2C DATA
	#define DAC8574_MS_UPDATE           (0x10) // UPDATE SELECTED DAC WITH I2C DATA (commonly utilized mode)
	#define DAC8574_MS_4CH_SYNCH_UPDATE (0x20) // Mode UPDATES ALL FOUR CHANNELS TOGETHER
	#define DAC8574_MS_BROADCAST_UPDATE (0x30) // Broadcast update mode ==> mode intended to enable up to 64 channels simultaneous update, if used the I2C broadcast address (0b10010000)
    // if SEL1 (BIT 2) = 0 all four channels are updated with the contents of their temporary register data
	// if SEL1 (BIT 2) = 1 all four channels are updated with the MS-BYTE and LS-BYTE data or power down
/*********************
    BIT 2 : Buff Sel1 Bit
    BIT 1 : Buff Sel0 Bit
    Channel Select Bits
**********************/
	#define DAC8574_CHANNEL_MASK        (0x06)
	#define chA           (0x00) // Channel A
	#define chB           (0x02) // Channel B
	#define chC           (0x04) // Channel C
	#define chD           (0x06) // Channel D

/*********************
    BIT 0 : PD0
    Power Down Flag
**********************/
	#define DAC8574_PD_MASK        		(0x01)
	#define DAC8574_PD_NORMAL           (0x00) // Normal operation
	#define DAC8574_PD_ACTIVATED        (0x01) // Power-down flag (MSB7 and MSB6 indicate a power-down operation)
/************************************************
    POWER-DOWN MODES of OPERATION FOR DAC8574
*************************************************/
	#define DAC8574_HIGH_IMPEDANCE_OUPUT (0x00)
	#define DAC8574_1kOhm_to_GND 		 (0x40)
	#define DAC8574_100kOhm_to_GND 		 (0x80)
	#define DAC8574_HIGH_IMPEDANCE 		 (0xC0)

 // Function prototypes
int init_DAC8574(uint8_t A3, uint8_t A2, uint8_t A1, uint8_t A0, uint8_t channel, uint8_t power_mode);
#endif /* SRC_DAC8574_CONFIGURATION_H_ */
