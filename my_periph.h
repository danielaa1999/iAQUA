/* ----- ---- -------- -----------------------------------------------
* 1.00  jcm  13/07/22 First release
* 2.00  psl  29/05/23 Remove functions and changes
* 3.00 dpg 07/02/2025 migrated to linux
******************************************************************************/
#ifndef MY_PERIPH_H_   /* Include guard */
#define MY_PERIPH_H_
/***************************** Include Files *********************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/gpio.h>
#include <string.h>
/************************** Constant Definitions *****************************/
#define GPIO_CHIP_DEV "/sys/class/gpio"
#define SRQ_CHANNEL "1009" //input, controlled by iAQUA chips
#define SRQ_LED_CHANNEL "1010"//output

/************************** Function Prototypes ******************************/
int GPIO_Init();
uint8_t SRQ_Read();
void SRQ_LED_Write(uint8_t data);

#endif // MY_PERIPH_H_
