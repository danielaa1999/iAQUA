/*
 * my_periph.c
 *
 * Created on: 29 may. 2023
 * Author: psl
 * C file which contains the configuration of the SRQ line
 */

/***************************** Include Files *********************************/
#include "my_periph.h"
/************************** Function Definitions *****************************/
  int  exportfd_srq, directionfd_srq,valuefd_srq;  // File descriptor for reading SRQ value

/*****************************************
 FUNCTION int GPIO_Init()

 1) Purpose and functionality : Function initializes the GPIO of the SRQ line
 2) Preconditions and limitations : To set the SRQ as an INPUT, need the SRQ channel
 3) Status label:Tested and works
 ******************************************/
  int GPIO_Init()
  {
      //Configure the GPIO for reading SRQ
      exportfd_srq = open("/sys/class/gpio/export", O_WRONLY);
      if (exportfd_srq < 0)
      {
          perror("Cannot open GPIO to export it");
          exit(1);
      }
      // Export SRQ_CHANNEL
      write(exportfd_srq, SRQ_CHANNEL, strlen(SRQ_CHANNEL));
      close(exportfd_srq);
      // Configure SRQ_CHANNEL as input
      directionfd_srq = open("/sys/class/gpio/gpio1007/direction", O_RDONLY);
      if (directionfd_srq < 0)
      {
    	  perror("Cannot open GPIO direction for SRQ_CHANNEL");
    	  exit(1);
      }
      write(directionfd_srq, "in", 3);
      close(directionfd_srq);
      // Configure to get the value of the SRQ
      valuefd_srq = open("/sys/class/gpio/gpio1007/value", O_RDONLY);
      if (valuefd_srq < 0) {
    	  perror("Cannot open GPIO value for SRQ");
    	  return EXIT_FAILURE;
      }
      return EXIT_SUCCESS; // Return success
      }
/*****************************************
 FUNCTION uint8_t SRQ_Read()

 1) Purpose and functionality : Function reads the state of the SRQ channel
 2) Preconditions and limitations : Need the SRQ channel configured as input
 3) Status label: Tested and works
 ******************************************/
  uint8_t SRQ_Read()
  {
      char value;
      // Reset file pointer to start of the file before each read
      lseek(valuefd_srq, 0, SEEK_SET);
      // Read SRQ value
      if (read(valuefd_srq, &value, 1) != 1) {
          perror("Failed to read SRQ value");
          return 0xFF;  // Error code or unexpected result
      }
      // Convert character ('0' or '1') to integer
      return (value == '1') ? 1 : 0;
  }
