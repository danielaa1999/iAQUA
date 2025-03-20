/*
 * ADS1100_configuration.h
 *
 *  Created on: 29 may. 2023
 *      Author: psl
 *
 *      Header file containing the configuration of the adc
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include "../my_periph/my_periph.h"

/**************************************************************************
    ADS1100_configuation.h
**************************************************************************/
#ifndef SRC_ADS1100_CONFIGURATION_H_
#define SRC_ADS1100_CONFIGURATION_H_

/************************************************
    ADS1100 I2C ADDRESSES
*************************************************/
 #define ADS1100_DEFAULT_ADDRESS   (0b1001)  //1001
// 1001 000 (ADDR = GND) depending of the three last bits set at the factory
typedef uint8_t AddressType;

/************************************************
    CONVERSION DELAY (in mS)
*************************************************/
    #define ADS1100_CONVERSIONDELAY         (1000) // => 1s

/*****************************************
    BIT 7: ST/BSY
******************************************/
    #define ADS1100_OS_MASK      (0x80)      // Conversion

/***********************
    SINGLE CONVERSION MODE
************************/
    #define ADS1100_OS_NOEFFECT  (0x00)      // Write: Bit = 0 ==> NO EFFECT
    #define ADS1100_OS_SINGLE    (0x80)      // Write: Bit = 1 ==> CAUSES CONVERSION TO START (DEFAULT)
    #define ADS1100_OS_BUSY      (0x00)      // Read: Bit = 0 Device is not performing a conversion
    #define ADS1100_OS_NOTBUSY   (0x80)      // Read: Bit = 1 Device is busy performing a conversion

/***********************
    CONTINUOUS CONVERSION MODE
************************/
//ADS1100 ignores the written value of ST/BSY


/*****************************************
    BIT 6-5: RESERVED
******************************************/
// BITS 6 and 5 must be set to zero

/*****************************************
    BIT 4: SC
******************************************/
    #define ADS1100_MODE_MASK        (0x10)      // Device operating mode
/***********************
    SET to CONTINUOUS CONVERSION MODE
************************/
    #define ADS1100_MODE_CONTINUOUS  (0x00)      // Continuous conversion mode (DEFAULT)
/***********************
    SET to SINGLE CONVERSION MODE
************************/
    #define ADS1100_MODE_SINGLE      (0x10)      // Single-conversion mode

/*****************************************
    BITS 3-2: DR
    TABLE VI. DR BITS
******************************************/
    #define ADS1100_DR_MASK   (0x0C)      // Data rate
    #define ADS1100_DR_128    (0x00)      // 128 SPS
    #define ADS1100_DR_32     (0x04)      // 32 SPS
    #define ADS1100_DR_16     (0x08)      // 16 SPS
    #define ADS1100_DR_8      (0x0C)      // 8 SPS (default)

/*****************************************
    BITS 1-0: DR
    TABLE VII. PGA BITS
******************************************/
    #define ADS1100_PGA_MASK     (0x03)      // Programmable gain amplifier configuration
    #define ADS1100_PGA_1        (0x00)      // G=1 (default)
    #define ADS1100_PGA_2        (0x01)      // G=2
    #define ADS1100_PGA_4        (0x02)      // G=4
    #define ADS1100_PGA_8        (0x03)      // G=8

typedef struct conf_ads conf_ads;
struct conf_ads{
	uint32_t data_rate;
	uint32_t pga;
	uint32_t set_mode;
	uint32_t write_read;
};
/*DEFINE*/
#define WRITE_BYTES_ADC  	1
#define READ_BYTES_ADC    	3
#define IIC_SCLK_RATE		100000 //100kHz
#define I2C_SLAVE_FORCE 0x0706
#define I2C_SLAVE    0x0703    /* Change slave address*/
#define I2C_FUNCS    0x0705    /* Get the adapter functionality */
#define I2C_RDWR    0x0707    /* Combined R/W transfer (one stop only)*/
#define PAGESIZE                16

uint16_t init_ADS1100_BUSA(uint8_t address_ads);//
uint16_t init_ADS1100_BUSB(uint8_t address);

#endif /* SRC_ADS1100_CONFIGURATION_H_ */


