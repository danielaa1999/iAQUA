/*
 * DAC8574_configuration.c
 *
 *  Created on: 30 may. 2023
 *      Author: Pauline SINQUIN--L'HARIDON
 *      Modified: Daniela Peña Gómez
 *      C file containing the configuration of the DAC
 */
#include "DAC8574_configuration.h"

uint8_t WriteBuffDAC[WRITE_BUFF_DAC];
uint8_t ReadBuffDAC[READ_BUFF_DAC];	/* Read buffer for reading a page. */
const uint8_t VPOT_WE, VPOT_RE, VREF_ORP, RE;
int fd_DAC,ret;
typedef uint8_t AddressType;

/*****************************************
   	 FUNCTION int init_DAC8574(uint8_t A3, uint8_t A2, uint8_t A1, uint8_t A0, uint8_t channel, uint8_t power_mode)

   	 1) Purpose and functionality : function initializing the Digital-To-Analog Converter for one channel, performing a write
     2) Preconditions and limitations : need the address of the DAC, need to have the ID of the channel A, need to know if it perform a power down or a normal operation
     3) Status label: PASSED TEST
******************************************/
int init_DAC8574(uint8_t A3, uint8_t A2, uint8_t A1, uint8_t A0, uint8_t channel, uint8_t power_mode){
	int Check;
	uint8_t address_dac;
	uint8_t CONTROLBYTE=0;
	for (int i=0; i<WRITE_BUFF_DAC; i++){
			WriteBuffDAC[i]=0;
		}
	//Initialize device
	const char *path_i2c_A=strdup("/dev/i2c-2");
	address_dac = (DAC8574_DEFAULT_ADDRESS << 2) | (A1 << 1) | (A0 << 0) ;
	fd_DAC = open(path_i2c_A, O_RDWR);
	if (fd_DAC < 0) {
	        printf( "Tried to open '%s'", path_i2c_A);
	        return EXIT_FAILURE;
	    }
	    ret = ioctl(fd_DAC, I2C_SLAVE, address_dac);
	    if (ret < 0) {
	        perror("Failed to set I2C slave address");
	        close(fd_DAC);
	        return ret;
	    }
	    CONTROLBYTE= (A3<<7)|(A2<<6)|(DAC8574_MS_UPDATE)|(channel)|(power_mode<<0);
	    WriteBuffDAC[0]=CONTROLBYTE;
	    if (power_mode==DAC8574_PD_ACTIVATED){
	    	WriteBuffDAC[1]=DAC8574_HIGH_IMPEDANCE;
	    	WriteBuffDAC[2]=0b00000000;
	    }
	    else if (power_mode==DAC8574_PD_NORMAL)
	    {
	    	WriteBuffDAC[1]=0x00;				//value to change
	    	WriteBuffDAC[2]=0x00;				//value to change
	    	//0xFFFF -> 1.2 V          0x7FFF -> 0.6V        0x0000 -> 0V
	    	}
	    else {EXIT_FAILURE;
	    close(fd_DAC);
	    }
	    /* Write  put a voltage in the DAC */
	    Check = write(fd_DAC, WriteBuffDAC, WRITE_BUFF_DAC);
	    if (Check != WRITE_BUFF_DAC) {
	    	perror("IIC write failed!");
	    	close(fd_DAC);
	    	return EXIT_FAILURE;
	    }
	    close(fd_DAC);
	    return EXIT_SUCCESS;
}
