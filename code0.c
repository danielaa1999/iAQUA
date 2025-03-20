#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/i2c-dev.h>
#include <linux/gpio.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <math.h>
#include "i2c_custom_C/I2C_AER.h"
#include "ADS1100_configuration/ADS1100_configuration.h"
#include "gpio_configuration/gpio_configuration.h"
#include "my_periph/my_periph.h"
#include "DAC8574_configuration/DAC8574_configuration.h"
#include "mqtt/define_buffer.h"

//General initialice
uint8_t type_sensor_address_IMB_BUSB[8]={ISFET1, ISFET2,ISFET3, ISFET4,ISFET5, ISFET6,ISFET7};
uint8_t type_sensor_address_IMB_BUSA[2]={AMP3, ORP};
uint8_t channel[4]={chA, chB, chC, chD}; // channels of the DAC
extern volatile uint8_t AMP_state;
int16_t data_adc = 0;
bool init_iaqua = false;
bool init_iaqua2 = false;
bool init_iaqua3 = false;
//Set up MQTT clients
void setup_mqtt(args_displayInfo* params) {
	int rc;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	MQTTClient_create(&params->client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (MQTTClient_connect(params->client, &conn_opts) != MQTTCLIENT_SUCCESS) {
		printf("Failed to connect to MQTT broker\n");
		exit(EXIT_FAILURE);
	}
	// Define the MQTT topic as a variable and subscribe
	char *topic2 = "MQTTExamples/Date";
	if ((rc = MQTTClient_setCallbacks(params->client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS) {
		printf("Failed to set callbacks, return code %d\n", rc);
		MQTTClient_destroy(&params->client);
		exit(EXIT_FAILURE);
	}
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((rc = MQTTClient_connect(params->client, &conn_opts)) != MQTTCLIENT_SUCCESS)
	{
		printf("Failed to connect, return code %d\n", rc);
		MQTTClient_destroy(&params->client);
		exit(EXIT_FAILURE);
	}
	if ((rc = MQTTClient_subscribe(params->client, topic2, QOS)) != MQTTCLIENT_SUCCESS)
	{
		printf("Failed to subscribe, return code %d\n", rc);
		MQTTClient_disconnect(params->client, 10000);
		MQTTClient_destroy(&params->client);
		exit(EXIT_FAILURE);
	}
	while (strlen(last_message) == 0) {
		usleep(100); // Sleep for 100ms to avoid busy-waiting
		}
	// Process the received MQTT message to create a file
	char filename2[256];
	char fullpath[512];
	strncpy(filename2, last_message, sizeof(filename2));
	cleanup_filename(filename2);
	const char* filepath2 = "/run/media/mmcblk0p1";
	create_file(filepath2, filename2,fullpath);
	// Print the name of the created file
	printf("Name of the file created: %s/%s.txt\n", filepath2, filename2);
	params->file2 = fopen(fullpath, "w");
	if (params->file2 == NULL) {
		perror("Failed to open file for writing");
		printf("File path: %s\n", fullpath);
		exit(EXIT_FAILURE);
	}
	// Cleanup MQTT client
	if ((rc = MQTTClient_unsubscribe(params->client, topic2)) != MQTTCLIENT_SUCCESS) {
		printf("Failed to unsubscribe, return code %d\n", rc);
	} else {
		printf("Successfully unsubscribed from topic: %s\n", topic2);
	}
}
// Function to copy matrix 1 to matrix2 and then reseting matrix1, matrix2 is used to avoid data races with the thread that reads the iAQUA data.
void reset_value_matrix(args_displayInfo* params) {
	// Copy value_matrix1 to value_matrix2
	for (int i = 0; i < 77; i++) {
		for (int j = 0; j < 4; j++) {  // Copy all 4 columns
			params->value_matrix2[i][j] = params->value_matrix[i][j];
		}
	}
	pthread_mutex_lock(&params->matrix1_lock);
	// Reset value_matrix1
	for (int i = 0; i < 77; i++) {
		for (int j = 0; j < 4; j++) {
			params->value_matrix[i][j] = 0;
		}
	}
	pthread_mutex_unlock(&params->matrix1_lock);
}
// Function to set up I2C and GPIO configurations (I2C custom)
void initialize_i2c_custom(args_displayInfo* params) {
    // Initialize GPIO and I2C
    if (GPIO_Init() != EXIT_SUCCESS) {
        exit(EXIT_FAILURE);
    }
    Init_I2C();
    for (int i = 0; i < 7; i++) {
        params->address_chip_TSMC[i] = 0;// Initialize address_chip_TSMC to zero
    }
    for (int i = 0; i < 77; i++) {/// initialice look table and value matrix 1 and 2 to 0
          params->lookup_tab.address_chip_tab[i] = 0;
          params->lookup_tab.sensor_tab[i] = 0;
          params->lookup_tab.prio_tab[i] = 0;
          for (int j = 0; j < 4; j++) {
              params->value_matrix[i][j] = 0;
              params->value_matrix2[i][j] = 0;
          }
      }
    int k = 0;  // Counter for acknowledged chip addresses
    for (uint8_t j = 1; j < 128; j++) {// Scan all addresses from 1 to 127
        if (Check_ACK_I2C(j) == 1) { // If device at address j acknowledges
            params->address_chip_TSMC[k] = j;  // Save acknowledged address
            printf("Chip %d: address %i\n\r", k, params->address_chip_TSMC[k]);
            k++;
            if (k >= 7) {  // Limit to the first 7 acknowledged addresses
                break;
            }
        }
    }
    int start_index = 0;  // Start index for the sensor range
    for (int i = 0; i < k; i++) {  // Only iterate up to the number of chips acknowledged
           if (Check_ACK_I2C(params->address_chip_TSMC[i]) == 1) {
               uint8_t chip_address = params->address_chip_TSMC[i];
               printf("Initializing chip at address %d\n", chip_address);
               // Initialize PRIO and sensors (starting with a chunk based on the start_index)
               disable_sensor(chip_address);
               init_PRIO(params, chip_address, start_index);
               printf("Initialized PRIO for chip address %d\n", chip_address);
               start_index += 10;  // Move to the next chunk for the next chip
           }
       }
}
// Function to initialize the ADC and DAC for sensors (I2C standard B)
void initialize_i2c_standard(args_displayInfo* params) {
    for (int i = 0; i < 7; i++) {
        params->address_adc_BUSB[i] = i;
        params->current_value_imb[i][0].sensor = i;//the value of the first column is the number of the sensor
        params->current_value_imb[i][1].data = 0; //the value of the data initialiced as 0
        params->current_value_imb[i][2].time = 0; //the value of timestamp initialiced as 0
        params->sensor_gains_offsets[i] = HARD_CODED_SENSOR_GAINS_OFFSETS[i];
    }  for (int i = 0; i < 2; i++) {
        params -> current_value_imb[i + 7][0].sensor =i+7;
        params->current_value_imb[i+7][1].data = 0;
        params->current_value_imb[i+7][2].time = 0; //the value of the data initialiced as 0
        params->sensor_gains_offsets[i+7] = HARD_CODED_SENSOR_GAINS_OFFSETS[i+7];
    }
    init_gpio(&params->gpio_config);//initialice IMB gpio as output
    disable_enable_channel(&params->gpio_config, 0X00);//Give value to the dis_imb meaning power
}
//Thread to read imb sensors and to write the data in the microsd
void* sensor_imb(void* args) {
    args_displayInfo* params = (args_displayInfo*)args;
    long long delta_time = 0;
    long long current_time_imb = 0;
    char binary_string_imb[65];
    char checksum_imb[9];
    uint8_t chip_address=0;
    uint8_t board_type=0;
    uint32_t time_imb=0;
    int check;
    while (1) {
    	check= init_DAC8574(0b0, 0b0, 0b0, 0b1, chA, DAC8574_PD_ACTIVATED);
    	 if (check != EXIT_SUCCESS) {
    	    	EXIT_FAILURE;
    	    }
        // Get the current timestamp (milliseconds)

    	 current_time_imb = current_timestamp();
        delta_time = current_time_imb - params->start_program; // Calculate delta_time based on the program start time
        time_imb=(int32_t)delta_time;
        // Process each of the sensors
        for (int i = 0; i < 7; i++) {
            if ((read_channel(&params->gpio_config) & (1 << i)) == 0) {
                int16_t data_adc = init_ADS1100_BUSB(params->address_adc_BUSB[i]);
              //  pthread_mutex_lock(&params->imb_lock);
                params->current_value_imb[i][1].data = data_adc;  // data
                params->current_value_imb[i][0].sensor = i;        // sensor type
                params->current_value_imb[i][2].time = time_imb;  // Timestamp
               // pthread_mutex_unlock(&params->imb_lock);
            }
            build_binary_string(binary_string_imb,time_imb, chip_address, board_type,i, data_adc);
            calculate_checksum(binary_string_imb, checksum_imb);
            fprintf(params->file2, "%s %s\n",binary_string_imb, checksum_imb);
        }
        // AMP of IMB PCB
        if ((read_channel(&params->gpio_config) & (1 << 7)) == 0) {
            int16_t data_adc = init_ADS1100_BUSA(params->address_adc_BUSA[0]);
            //pthread_mutex_lock(&params->imb_lock);
            params->current_value_imb[7][1].data = data_adc;
            params->current_value_imb[7][0].sensor = 7;
            params->current_value_imb[7][2].time = time_imb;
           // pthread_mutex_unlock(&params->imb_lock);
            build_binary_string(binary_string_imb,time_imb, chip_address, board_type, 7,data_adc);
            calculate_checksum(binary_string_imb, checksum_imb);
            fprintf(params->file2, "%s %s\n",binary_string_imb, checksum_imb);
        }
        // ORP of IMB PCB
        if ((read_channel(&params->gpio_config) & (1 << 8)) == 0) {
            int16_t data_adc = init_ADS1100_BUSA(params->address_adc_BUSA[1]);
           // pthread_mutex_lock(&params->imb_lock);
            params->current_value_imb[8][1].data = data_adc;
            params->current_value_imb[8][0].sensor = 8;
            params->current_value_imb[8][2].time = time_imb;
            //pthread_mutex_unlock(&params->imb_lock);
            build_binary_string(binary_string_imb,time_imb, chip_address, board_type, 8, data_adc);
            calculate_checksum(binary_string_imb, checksum_imb);
            fprintf(params->file2, "%s %s\n",binary_string_imb, checksum_imb);
        }
    }
    pthread_exit(NULL);
}


//Thread to read iAQUA sensors
void* sensor_iaqua(void* args) {
    args_displayInfo* params = (args_displayInfo*)args;
    int check;

    check = init_DAC8574(0b0, 0b0, 0b0, 0b0, chA, DAC8574_PD_ACTIVATED); //high impedance
    if (check != EXIT_SUCCESS) {
    	EXIT_FAILURE;
    }
    check = init_DAC8574(0b0, 0b0, 0b0, 0b0, chB, DAC8574_PD_ACTIVATED);//high impedance
    if (check != EXIT_SUCCESS) {
    	EXIT_FAILURE;
    }
    check = init_DAC8574(0b0, 0b0, 0b0, 0b0, chC, DAC8574_PD_ACTIVATED);//high impedance
    if (check != EXIT_SUCCESS) {
    	EXIT_FAILURE;
    }
    check = init_DAC8574(0b0, 0b0, 0b0, 0b0, chD, DAC8574_PD_ACTIVATED);//high impedance
    if (check != EXIT_SUCCESS) {
    	EXIT_FAILURE;
    }
    long long current_time_imb = 0;
    uint32_t timestamp=0;
    long long delta_time = 0;
    while (1) {
        uint16_t data_sensor=0;
        uint8_t sensor_data=0;
        uint8_t priority_index=0;
        char binary_string_iaqua[65];
        char checksum[9];
        uint8_t chip_address=0;
        uint8_t sensor=0;
        int board=1;
        if (!SRQ_Read()){/// Collect data for iAQUA PCB if the SRQ line is pulled low
        	current_time_imb = current_timestamp();
        	delta_time = current_time_imb - params->start_program; // Calculate delta_time based on the program start time
        	data_sensor = Event_I2C();
        	priority_index = (uint8_t)((data_sensor & (0xFF00)) >> 8);
        	sensor_data = (uint8_t)(data_sensor & 0x00FF);
        	uint8_t chip_index = 0xFF; // Initialize to an invalid value to detect issues later
            if (priority_index>77){//If the priority index is invalid skip the code
                        	printf ("Priority index %d\n",priority_index);
                        	continue;
                        }
           // if (sensor_data != 1 && sensor_data != 2) {//Check if the data of a sensor is incorrect
                    //uint8_t chip_address = params->lookup_tab.address_chip_tab[priority_index];
                    //uint8_t sensor_index = params->lookup_tab.sensor_tab[priority_index];
                   //disable_sensor2(chip_address, sensor_index);//disable that sensor
                    //continue;// Skip the rest of the function and continue with the next loop iteration
          //  }
            	for (uint8_t i = 0; i < 7; i++) {//get an index for each chip address to place the data in the buffer it goes from lower to higher chip address
            	    if (params->address_chip_TSMC[i] == params->lookup_tab.address_chip_tab[priority_index]) {
            	       chip_index = i; // Assign the matching index
            	        break;          // Exit the loop once a match is found
            	    }
            	}
            	if (chip_index == 0xFF) {
            	    continue;
            	}
            	// Mark as first data receive
            	if (params->value_matrix[priority_index][3] == 0) {
            		params->value_matrix[priority_index][3] = 1;
            	}
            	//Place the data in matrix1
            	if (sensor_data == 2) {
            		pthread_mutex_lock(&params->matrix1_lock);
            		params->value_matrix[priority_index][0]=params->lookup_tab.sensor_tab[priority_index];
            		params->value_matrix[priority_index][1]=chip_index;
            		params->value_matrix[priority_index][2] -= 1.0;
            		pthread_mutex_unlock(&params->matrix1_lock);
            	} else if (sensor_data == 1) {
            		pthread_mutex_lock(&params->matrix1_lock);
            		params->value_matrix[priority_index][0]=params->lookup_tab.sensor_tab[priority_index];
            		params->value_matrix[priority_index][1]=chip_index;
            		params->value_matrix[priority_index][2] += 1.0;
            		pthread_mutex_unlock(&params->matrix1_lock);
            	}
            	sensor=params->lookup_tab.sensor_tab[priority_index];
            	chip_address=params->lookup_tab.address_chip_tab[priority_index];
            	timestamp=(int32_t)delta_time;;
            	build_binary_string(binary_string_iaqua,timestamp, chip_address, board, sensor,sensor_data);
            	calculate_checksum(binary_string_iaqua, checksum);
            	fprintf(params->file2, "%s %s\n",binary_string_iaqua, checksum);
        }
        else {
        	continue;//	If SRQ line is low skip
        	}
    }
    pthread_exit(NULL);
}
//Thread to enable iaqua sensors and to write the data in the microsd
void* timer_gen_thread(void *args) {
    args_displayInfo* params = (args_displayInfo*)args;
    while (1) {
        long long current_time_iaqua = current_timestamp();
        if (current_time_iaqua - params->start_program >= 10000 && init_iaqua == false) {
            for (int i = 0; i < 7; i++) {
                if (params->address_chip_TSMC[i] != 0) {
                    init_sensor(params->address_chip_TSMC[i], 0, 1);
                }
            }
            init_iaqua = true;
        }
        if (current_time_iaqua - params->start_program >= 30000 && init_iaqua2 == false) { // Turn ON ACQ
            for (int i = 0; i < 7; i++) {
                if (params->address_chip_TSMC[i] != 0) {
                    init_sensor2(params->address_chip_TSMC[i], 2, 2);
                }
            }
            init_iaqua2 = true;
        }
        if (current_time_iaqua - params->start_program >= 60000 && init_iaqua3 == false) { // Turn on ADC
            for (int i = 0; i < 7; i++) {
                if (params->address_chip_TSMC[i] != 0) {
                    init_sensor2(params->address_chip_TSMC[i], 3, 3);
                }
            }
            init_iaqua3 = true;
        }
    }
}
int main() {
	args_displayInfo params;
	int mqtt_initialized = 0;
    params.address_adc_BUSA[0] = 0b1001010;
    params.address_adc_BUSA[1] = 0b1001011;
    params.last_reset_time_iaqua = 0;
    params.last_reset_time_imb = 0;
    pthread_mutex_init(&params.matrix1_lock, NULL);
    pthread_mutex_init(&params.imb_lock, NULL);
    // Allocate and initialize the sensor buffer and other resources
    params.pointread = (SensorBufferSlot**)malloc(sizeof(SensorBufferSlot*));
    params.pointwrite = (SensorBufferSlot**)malloc(sizeof(SensorBufferSlot*));
    params.sensor_buffer = (SensorBufferSlot*)malloc(BUFFER_SIZE * sizeof(SensorBufferSlot));
    if (!params.sensor_buffer) {
        perror("Failed to allocate memory for sensor buffer");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < BUFFER_SIZE; i++) {//Set the buffer to 0
        memset(params.sensor_buffer[i].sensor_data_i, 0, sizeof(params.sensor_buffer[i].sensor_data_i));
    }
    if (!mqtt_initialized) {//Only intialice mqtt client once
            setup_mqtt(&params);
            mqtt_initialized = 1;
        }
    initialize_i2c_standard(&params);
    initialize_i2c_custom(&params);
    // Initialize gain and offset iAQUA to default values.
    for (int chip = 0; chip < 128; chip++) {
        for (int sensor = 0; sensor < 11; sensor++) {
            for (int polarity = 0; polarity < 2; polarity++) {
                if (HARD_CODED_GAIN_OFFSET_MATRIX[chip][sensor][polarity].a != 0.0 ||
                    HARD_CODED_GAIN_OFFSET_MATRIX[chip][sensor][polarity].b != 0.0) {
                    params.gain_offset_matrix[chip][sensor][polarity] = HARD_CODED_GAIN_OFFSET_MATRIX[chip][sensor][polarity];
                } else {
                    params.gain_offset_matrix[chip][sensor][polarity].a = 1.0;  // Default slope
                    params.gain_offset_matrix[chip][sensor][polarity].b = 0.0;  // Default intercept
                }
            }
        }
    }
    params.program_start_time = (uint32_t)(current_timestamp());
    params.start_program =current_timestamp();
    pthread_t read_thread, write_thread,sensor_imb_thread,sensor_iaqua_thread,timer_thread;
    pthread_create(&sensor_iaqua_thread, NULL, sensor_iaqua, (void*)&params);
    pthread_create(&sensor_imb_thread, NULL, sensor_imb, (void*)&params);
    pthread_create(&write_thread, NULL, write_data_thread, (void*)&params);
    pthread_create(&read_thread, NULL, buffer_read_thread, (void*)&params);
    pthread_create(&timer_thread, NULL, timer_gen_thread, (void*)&params);
    pthread_join(write_thread, NULL);
    pthread_join(timer_thread, NULL);
    pthread_join(sensor_iaqua_thread, NULL);
    pthread_join(sensor_imb_thread, NULL);
    pthread_join(read_thread, NULL);
    free(params.sensor_buffer);
    pthread_exit(NULL);
}
