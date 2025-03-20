/*
 * ADS1100_configuration.c
 *
 *  Created on: 13 feb. 2024 -
 *      This C file contains the configuration of the ADC of the BUS B (ISFETs)
 */

/*****************************************
    INCLUDE
******************************************/

#include "../ADS1100_configuration.h"

/*****************************************
    VARIABLE DECLARATION
******************************************/
 uint8_t WriteBuff_B[WRITE_BYTES_ADC];
 uint8_t ReadBuff_B[READ_BYTES_ADC];
uint8_t setup_B = 0;
extern unsigned SentByteCount;
int i2c_fd_B;

/*****************************************
    FUNCTION init_ADS1100_BUSB(u8 address_ads)

    1) Purpose and functionality : function initializing the Analog-To-Digital Converter (ISFETs) for the channel B, performing a write and a read
    2) Preconditions and limitations : need the address of the ADC, need to have the ID of the channel B
    3) Status label: Works just fine
******************************************/
    uint16_t init_ADS1100_BUSB(uint8_t address) {
        int status;
        uint8_t address_ads;
        const char *i2c_device = "/dev/i2c-3";
        address_ads = (ADS1100_DEFAULT_ADDRESS << 3) |(address<<0); // Combine default address and input address
        // Open I2C device
        i2c_fd_B = open(i2c_device, O_RDWR);
        if (i2c_fd_B < 0) {
            perror("Failed to open the I2C device");
            return EXIT_FAILURE;
        }
        // Set I2C address
        if (ioctl(i2c_fd_B, I2C_SLAVE, address_ads) < 0) {
            perror("Failed to set I2C address");
            close(i2c_fd_B);
            return EXIT_FAILURE;
        }
        // Configure the ADS1100 control byte
        conf_ads test;
        test.data_rate = ADS1100_DR_8;
        test.pga = ADS1100_PGA_1;
        test.set_mode = ADS1100_MODE_CONTINUOUS;
        setup_B&=~(1<<6);
        setup_B&=~(1<<5);
        setup_B|=test.set_mode;
        setup_B|=test.data_rate;
        setup_B|=test.pga;

        // Write to the ADC device
        WriteBuff_B[0] = setup_B;
        status = write(i2c_fd_B, WriteBuff_B, WRITE_BYTES_ADC);
        if (status != WRITE_BYTES_ADC) {
            perror("IIC write failed!");  // This will provide a detailed error message
            close(i2c_fd_B);
            return EXIT_FAILURE;
        }
        // Small delay for device readiness if needed
       // usleep(5000);
        // Read from ADS1100
        ReadBuff_B[2]=setup_B;
        status = read(i2c_fd_B, ReadBuff_B, READ_BYTES_ADC);
        if (status != READ_BYTES_ADC) {
            perror("IIC read failed!");  // This will provide a detailed error message
            close(i2c_fd_B);
            return EXIT_FAILURE;
        }

        int16_t adc_data = (ReadBuff_B[0] << 8) | ReadBuff_B[1];
        if (adc_data & 0x8000) {  // Check if the sign bit (MSB) is set
            adc_data |= 0xFFFF0000; // Extend the sign by setting the upper 16 bits to 1
        }

        close(i2c_fd_B);// Close the I2C file descriptor
        return adc_data;//Send the data
    }

