/*
 * gpio_configuration.h
 *
 *  Created on: 30 may. 2023
 *      Author: psl
 *
 *      Header file for GPIO_CONFIGURATION
 */
#ifndef SRC_GPIO_CONFIGURATION_GPIO_CONFIGURATION_H_
#define SRC_GPIO_CONFIGURATION_GPIO_CONFIGURATION_H_
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/gpio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define IMB_GPIO_PATH "/sys/class/gpio"
typedef struct {
    char* gpio_pins[9];
    int value_fds[9];
} gpio_config_t;

void init_gpio(gpio_config_t* config);
void disable_enable_channel(gpio_config_t* config,uint16_t data);
uint16_t read_channel(gpio_config_t* config);

#endif /* SRC_GPIO_CONFIGURATION_GPIO_CONFIGURATION_H_ */
