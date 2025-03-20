/*
 * ADS1100_configuration.c
 *
 *  Created on: 13 feb. 2024 -
 *      This C file contains the configuration of the ADC of the BUS A (ORP and AMP)
 */

/*****************************************
    INCLUDE
******************************************/
#include "../ADS1100_configuration.h"

/*****************************************
    VARIABLE DECLARATION
******************************************/
uint8_t WriteBuff_A[WRITE_BYTES_ADC];
uint8_t ReadBuff_A[READ_BYTES_ADC];
uint8_t setup_A = 0;
int i2c_fd_A;

/*****************************************
    FUNCTION init_ADS1100_BUSA(u8 address_ads)

    1) Purpose and functionality : function initializing the Analog-To-Digital Converter (ISFETs) for the channel B, performing a write and a read
    2) Preconditions and limitations : need the address of the ADC, need to have the ID of the channel B
    3) Status label:Test and works
******************************************/
uint16_t init_ADS1100_BUSA(uint8_t address) {
	int status;
	uint8_t address_ads;
	const char *i2c_device = "/dev/i2c-2";
	address_ads = (ADS1100_DEFAULT_ADDRESS << 3) |(address<<0);// Combine default address and input address
	// Open I2C device
	i2c_fd_A = open(i2c_device, O_RDWR);
	if (i2c_fd_A < 0) {
		perror("Failed to open the I2C device");
		return EXIT_FAILURE;
        }
	// Set I2C address
	if (ioctl(i2c_fd_A, I2C_SLAVE, address_ads) < 0) {
		perror("Failed to set I2C address");
		close(i2c_fd_A);
		return EXIT_FAILURE;
	}
	// Configure the ADS1100 control byte
    conf_ads test;
    test.data_rate = ADS1100_DR_8;
    test.pga = ADS1100_PGA_2;
    test.set_mode = ADS1100_MODE_CONTINUOUS;
    setup_A&=~(1<<6);
    setup_A&=~(1<<5);
    setup_A|=test.set_mode;
    setup_A|=test.data_rate;
    setup_A|=test.pga;

    //Set up what to write  and read in the ADS
    WriteBuff_A[0] = setup_A;
    ReadBuff_A[2]=setup_A;
    // Write to the ADC device
    status = write(i2c_fd_A, WriteBuff_A, WRITE_BYTES_ADC);
     if (status != WRITE_BYTES_ADC) {
    	 fprintf(stderr, "IIC write failed! Error: %s (errno: %d)\n", strerror(errno), errno);
    	 close(i2c_fd_A);
    	 return EXIT_FAILURE;
     }
        usleep(5000);// Small delay for device readiness if needed
        // Read from ADS1100
        status = read(i2c_fd_A, ReadBuff_A, READ_BYTES_ADC);
        if (status != READ_BYTES_ADC) {
        	fprintf(stderr, "IIC write failed! Error: %s (errno: %d)\n", strerror(errno), errno);
        	close(i2c_fd_A);
            return EXIT_FAILURE;
        }
        int16_t adc_data = (ReadBuff_A[0] << 8) | ReadBuff_A[1];
        // If the result is negative (i.e., the sign bit is set), we need to sign-extend
        if (adc_data & 0x8000) {  // Check if the sign bit (MSB) is set
            adc_data |= 0xFFFF0000; // Extend the sign by setting the upper 16 bits to 1
        }
        // Close the I2C file descriptor
        close(i2c_fd_A);
        return adc_data;//return the data:)
    }
