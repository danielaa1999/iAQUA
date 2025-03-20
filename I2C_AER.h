/*****************************************************************************/
/**
*
* @file my_i2c.h
*
* MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- -----------------------------------------------
* 1.00  jcm  01/07/22 First release
* 2.00  psl  10/07/23 adding new functions
*3.00   DPG   07/02/2025 addapted
******************************************************************************/
#ifndef MY_I2C_H_   /* Include guard */
#define MY_I2C_H_
/***************************** Include Files *********************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include "../my_periph/my_periph.h"

/************************** Constant Definitions *****************************/
#define I2C_MASTER_BASE_ADDR	0x43C00000
// reg0
#define I2C_MASTER_REG0_ADDR	I2C_MASTER_BASE_ADDR + 0
#define CLEAR_OFFSET		0
#define ACK_TEST_OFFSET		1
#define ENABLE_OFFSET		2
#define ENABLE_EVENT_OFFSET	3
// reg1
#define I2C_MASTER_REG1_ADDR 	I2C_MASTER_BASE_ADDR + 4
#define RW_OFFSET			0
#define WRITE_DATA_OFFSET	1
#define REG_ADDR_OFFSET		9
#define DEVICE_ADDR_OFFSET	17
// reg2
#define I2C_MASTER_REG2_ADDR 	I2C_MASTER_BASE_ADDR + 8
#define CLK_DIVIDER_OFFSET	0
// reg3
#define I2C_MASTER_REG3_ADDR 	I2C_MASTER_BASE_ADDR + 12
#define BUSY_OFFSET			0
#define ACK_OFFSET			1
#define MISO_DATA_OFFSET	2
#define SLAVE_PRIO_OFFSET	10
//
#define BUSY_MASK			0x01
#define ACK_MASK			0x01
#define MISO_DATA_MASK		0xFF
#define SLAVE_PRIO_MASK		0xFF
// -----------------------------
//Chip address
#define Addr7_MASTER_GENERAL_CALL 	0b0000000
//Sensor address
#define Addr5_MASTER_GENERAL_CALL 	0b00000
#define ISFET1 						0b00001
#define ISFET2 						0b00010
#define ISFET3 						0b00011
#define ISFET4 						0b00100
#define ISFET5 						0b00101
#define ISFET6 						0b00110
#define ISFET7 						0b00111
#define ISFET8 						0b01000
#define ISFET9 						0b01001
#define AMP3   						0b01010
#define ORP    						0b01011
#define AMP2    					0b01111 //not used
// ------------------------------
// -----------------------------
#define regA   0b000
#define regB   0b001
#define regC   0b010
#define regD   0b011
#define regE   0b100
#define regF   0b101
#define regG   0b110
#define regH   0b111 //not used

struct _args_displayInfo;  // Declaration from define buffer
typedef struct _args_displayInfo args_displayInfo;
#define F_CLK 100e6			// AXI clock frequency
#define F_I2C 400e3			// Desired I2C frequency (min. 400 Hz). ( <---- EDIT HERE)
#define writeBYTES 4
#define readBYTES 3
/************************** Function Prototypes ******************************/
void Init_I2C();
uint8_t Check_ACK_I2C(uint8_t chip_addr);
void Write_I2C(uint8_t chip_addr, uint8_t sensor_addr, uint8_t reg_addr, uint8_t write_data);
uint8_t Read_I2C(uint8_t, uint8_t, uint8_t);
uint16_t Event_I2C();
void write_i2c(uint8_t address_chip, uint8_t address_sensor, uint8_t reg, uint8_t reg_data);
void init_sensor(uint8_t address_chip, int start_j, int end_j);
void init_sensor2(uint8_t address_chip, int start_j, int end_j);
void enable_ORP_AMP(uint8_t address_chip);
void init_PRIO(args_displayInfo* params, uint8_t address_chip,int start_index);
void disable_sensor(uint8_t address_chip);
void disable_sensor2(uint8_t chip_address, uint8_t sensor_index);
#endif // MY_I2C_H_
