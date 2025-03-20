#include "gpio_configuration.h"


int exportfd, directionfd;
int gpio_fd=-1;
/*****************************************
   	 FUNCTION void init_gpio(void)

   	 1) Purpose and functionality : Function function which initialized the GPIO
     2) Preconditions and limitations : need DIS_IMB_ID, DIS_IMB_CHANNEL
     3) Status label: Tested and works
******************************************/
void init_gpio(gpio_config_t* config) {
    // Define GPIO pin numbers
    config->gpio_pins[0] = "1015";
    config->gpio_pins[1] = "1016";
    config->gpio_pins[2] = "1017";
    config->gpio_pins[3] = "1018";
    config->gpio_pins[4] = "1019";
    config->gpio_pins[5] = "1020";
    config->gpio_pins[6] = "1021";
    config->gpio_pins[7] = "1022";
    config->gpio_pins[8] = "1023";

    int exportfd = open(IMB_GPIO_PATH "/export", O_WRONLY);
    if (exportfd < 0) {
        perror("Failed to open GPIO device");
        return;
    }
    // Export all GPIOs
    for (int i = 0; i < 9; i++) {
        write(exportfd, config->gpio_pins[i], 4);
    }
    close(exportfd);
    printf("GPIO of IMB board exported successfully\n");
    // Set the direction of all GPIOs to be outputs
    char gpio_path[50];
    for (int i = 0; i < 9; i++) {
        snprintf(gpio_path, sizeof(gpio_path), "/sys/class/gpio/gpio%s/direction", config->gpio_pins[i]);
        int directionfd = open(gpio_path, O_RDWR);
        if (directionfd < 0) {
            printf("Cannot open GPIO direction for GPIO %s\n", config->gpio_pins[i]);
            exit(1);
        }
        write(directionfd, "out", 4);
        close(directionfd);
    }
    printf("GPIO direction set as output successfully\n");
}
/*****************************************
   	 FUNCTION void disable_enable_channel(u16 data)

   	 1) Purpose and functionality : Function which writes the data to 1 if disabling or 0 to enabling
     2) Preconditions and limitations : need DIS_IMB_CHANNEL and to be configured before by void init_gpio(void)
     3) Status label:TESTED and works
******************************************/
void disable_enable_channel(gpio_config_t* config,uint16_t data) {
	    config->gpio_pins[0] = "1015";
	    config->gpio_pins[1] = "1016";
	    config->gpio_pins[2] = "1017";
	    config->gpio_pins[3] = "1018";
	    config->gpio_pins[4] = "1019";
	    config->gpio_pins[5] = "1020";
	    config->gpio_pins[6] = "1021";
	    config->gpio_pins[7] = "1022";
	    config->gpio_pins[8] = "1023";
	// Open the value file descriptors for all GPIOs
	 char gpio_path[50];
	    for (int i = 0; i < 9; i++) {
	        snprintf(gpio_path, sizeof(gpio_path), "/sys/class/gpio/gpio%s/value", config->gpio_pins[i]);
	        config->value_fds[i] = open(gpio_path, O_RDWR);
	        if (config->value_fds[i] < 0) {
	            printf("Cannot open GPIO value for GPIO %s\n", config->gpio_pins[i]);
	            exit(1);
	        }
}
	    for (int i = 0; i < 9; i++) {//Checks the data form main two see what do anable and disable
	            char value = (data >> i) & 1 ? '1' : '0';
	            if (write(config->value_fds[i], &value, 1) != 1) {
	                perror("write");
	                exit(EXIT_FAILURE);
	            }
}
}
/*****************************************
   	 FUNCTION uint16_T read_channel(void)

   	 1) Purpose and functionality :Checks to see if the gpio is enable in order to read from that ADS
     2) Preconditions and limitations : need DIS_IMB_CHANNEL and to be configured before by void init_gpio(void)
     3) Status label: Tested and works
******************************************/
uint16_t read_channel(gpio_config_t* config) {
    char value_str[3];
    uint16_t gpio_value = 0;
    for (int i = 0; i < 9; i++) {
        if (pread(config->value_fds[i], value_str, 2, 0) < 0) {
            perror("Failed to read GPIO value");
            return -1;
        }
        gpio_value |= (atoi(value_str) << i);
    }
    return gpio_value;
}
