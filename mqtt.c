#include "define_buffer.h"// Definitions of global variables
bool new_data = false;
char last_message[256] = {0};
volatile MQTTClient_deliveryToken deliveredtoken = 0;
pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_cond = PTHREAD_COND_INITIALIZER;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////Process input data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to set the last timestamp
long long current_timestamp() {
 struct timeval te;
 gettimeofday(&te, NULL); // get current time
 long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // calculate milliseconds
 return milliseconds;
}
void calculate_checksum(const char* line_i, char* checksum) {
   char sum[9] = "00000000";
   char chunk[9];
   int carry = 0;
   // Process the string 8 bits at a time (8 blocks of 8 bits each)
   for (int i = 0; i < 8; i++) {
       // Copy the next 8-bit chunk
       strncpy(chunk, line_i + (i * 8), 8);
       chunk[8] = '\0';

       // Add the chunk to the sum
       carry = 0;
       for (int j = 7; j >= 0; j--) {
           int bit_sum = (sum[j] - '0') + (chunk[j] - '0') + carry;
           sum[j] = (bit_sum % 2) + '0';
           carry = bit_sum / 2;
       }
       // Add the carry if any
       if (carry == 1) {
           for (int j = 7; j >= 0; j--) {
               if (sum[j] == '0') {
                   sum[j] = '1';
                   break;
               } else {
                   sum[j] = '0';
               }
           }
       }
   }
   // Calculate the 1's complement of the sum
   for (int i = 0; i < 8; i++) {
       sum[i] = (sum[i] == '0') ? '1' : '0';
   }
   // Copy the result to the checksum
   strncpy(checksum, sum, 8);
   checksum[8] = '\0'; // Ensure the checksum is null-terminated
}
void int_to_binary_string(uint32_t value, char *output, int bits) {
    output[bits] = '\0';  // Null-terminate the binary string
    for (int i = bits - 1; i >= 0; i--) {
        output[i] = (value & 1) ? '1' : '0';
        value >>= 1;
    }
}
void decimal_to_twos_complement_binary(int16_t value, char* binary_str) {
    uint16_t twos_complement_value;
    // Convert to two's complement representation if negative
    if (value < 0) {
        twos_complement_value = (uint16_t)((~(-value) + 1) & 0xFFFF); // Two's complement
    } else {
        twos_complement_value = (uint16_t)(value); // Direct representation for positive numbers
    }
    for (int i = 15; i >= 0; i--) {// Generate binary string (16 bits)
        binary_str[i] = (twos_complement_value & 1) ? '1' : '0';
        twos_complement_value >>= 1;
    }
}
// Helper function to build the binary string
void build_binary_string(char* binary_string, uint32_t timestamp, uint8_t chip_address, uint8_t board_type,int sensor_type_index, int data) {
	char timestamp_bin[33], chip_address_bin[9], board_type_bin[2], sensor_type_index_bin[8], data_bin[16];
	int_to_binary_string(timestamp, timestamp_bin, 32);
	int_to_binary_string(chip_address, chip_address_bin, 8);
	int_to_binary_string(board_type, board_type_bin, 1);
	int_to_binary_string(sensor_type_index, sensor_type_index_bin, 7);
	decimal_to_twos_complement_binary(data, data_bin);
	snprintf(binary_string, 65, "%s%s%s%s%s", timestamp_bin, chip_address_bin, board_type_bin, sensor_type_index_bin, data_bin);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////Buffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to write a complete line into the buffer
void* write_data_thread(void *args) {
    args_displayInfo* params = (args_displayInfo*)args;
    while (1) {
    	long long current_time = current_timestamp();
        //Write IMB data to the buffer every 2ms
    	if (current_time - params->last_reset_time_imb >= 2) {
    		params->last_reset_time_imb = current_timestamp(); // Update last reset time
    		for (int i = 0; i < 9; i++) {
    			//pthread_mutex_lock(&params->imb_lock);
    			int8_t sensor_type_index = params->current_value_imb[i][0].sensor;    // Accessing sensor
    			int16_t raw_data = params->current_value_imb[i][1].data;              // Accessing data
    			uint32_t timestamp = params->current_value_imb[i][2].time;
    			///pthread_mutex_unlock(&params->imb_lock);
    			int chip_address=0;
    			int board_type=0;
    			//Process data to write into the buffer
    			double gain = params->sensor_gains_offsets[i].gain;
    			double offset = params->sensor_gains_offsets[i].offset;
    			double data = (raw_data - offset) / gain;
    			data *= 1000.0;
    			int16_t result = ceil(data);
    			char binary_string[65];
    			build_binary_string(binary_string,timestamp, chip_address, board_type, sensor_type_index,result);
    			*params->pointwrite = &params->sensor_buffer[i];
    			strncpy((*params->pointwrite)->sensor_data_i, binary_string, sizeof((*params->pointwrite)->sensor_data_i)); // Copy binary data to buffer
    			}
    	}
        // iAQUA, copy matrix1 to matrix2, set matrix1 to 0, process the data
    	if (current_time - params->last_reset_time_iaqua >= 1000) {
    		params->last_reset_time_iaqua = current_timestamp();  // Update last reset time
    		reset_value_matrix(params);//reset matrix 1 and copy it to matrix2 which is the one being checked here
    		for (int i = 0; i < 77; i++) {
    			if (params->value_matrix2[i][3] == 1 && params->value_matrix2[i][0] > 0) {  // Check if first data flag is set
    				uint32_t timestamp2=params->last_reset_time_iaqua-params->start_program;
    				uint8_t sensor = params->value_matrix2[i][0];
    				uint8_t chip = params->value_matrix2[i][1];
    				int16_t raw_data_2 = params->value_matrix2[i][2];
    				uint8_t index = i + 9;
    				uint8_t board=1;
    				uint8_t chip_address = params->address_chip_TSMC[chip];
    				// Choose gain and offset based on the sign of the data
    				Calibration coeff;
    				if (raw_data_2 >= 0) {
    					coeff = params->gain_offset_matrix[chip_address][sensor][0];  // Positive calibration
    					} else {
    						coeff = params->gain_offset_matrix[chip_address][sensor][1];  // Negative calibration
    						}
    				double a = coeff.a;
    				double b = coeff.b;
    				// Compute linear regression estimate
    				double data_2 = (raw_data_2 - b)/a;
    				int16_t result_2 = ceil(data_2);
    				char binary_string3[65];
    				build_binary_string(binary_string3,timestamp2, chip_address, board, sensor,result_2);
    				*params->pointwrite = &params->sensor_buffer[index];
    				strncpy((*params->pointwrite)->sensor_data_i, binary_string3, sizeof((*params->pointwrite)->sensor_data_i)); // Copy binary data to buffer
    				params->value_matrix2[i][3] = 0;
    			} else {
    				continue;
           }
        }
    	}
    }
    pthread_exit(NULL);
}
//Function to read the buffer in the desired order, interpret the data and send it to be published
void* buffer_read_thread(void *args) {
	args_displayInfo* params = (args_displayInfo*)args;
	 while (1) {
   for (int i = 0; i < 86; i++) {// Iterate through each buffer slot
       long long start_time_slot = current_timestamp();// Start timestamp
       *params->pointread = &params->sensor_buffer[i];// Update the pointer to the current buffer slot
       if (strlen((*params->pointread)->sensor_data_i) != 0) {// Check if the sensor data in the buffer slot is not empty
    	new_data=true;
    	//Timestamp of 10ms
       long long end_time_slot = current_timestamp();
       long long elapsed_time_slot = end_time_slot - start_time_slot;// Calculate elapsed time for the current slot
       if (elapsed_time_slot < 10) {
    	   usleep((10 - elapsed_time_slot) * 1000);// Ensure the operation takes 10 milliseconds
       }
       SensorData* data = extract_sensor_data((*params->pointread)->sensor_data_i, params);
       if (!data) {
           printf("Failed to extract sensor data from buffer slot %d\n", i);
           memset((*params->pointread)->sensor_data_i, 0, sizeof((*params->pointread)->sensor_data_i));
           continue;
       }
       publishing(data,params->client,TOPIC);// Call the publishing function after writing to the buffer
       if (i >= 0 && i <= 86) {
           memset((*params->pointread)->sensor_data_i, 0, sizeof((*params->pointread)->sensor_data_i)); // Set to 0 interpreted data
       }
       free(data->sensor_type);// Free the allocated memory for sensor data
       free(data);
   }
   }
	 }
	 pthread_exit((void *)NULL);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////Interpret data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int binary_to_decimal_time(const char* binary_timestamp, int size) {
   return strtol(strndup(binary_timestamp, size), NULL, 2);
}
long long binary_to_decimal(char* binary_str, int start, int end) {
   long long decimal_value = 0;
   for (int i = start; i <= end; i++) {
       decimal_value = (decimal_value << 1) | (binary_str[i] - '0');
   }
   return decimal_value;
}
double binary_to_decimal_sensor_data(char* binary_data, int size) {
   char value_str[16]; // Extract the 16 bits from the binary data
   strncpy(value_str, binary_data, 16);
   if (value_str[0] == '1') {// Check if the value is negative (assuming the first bit represents the sign)
       unsigned short int raw_value = (unsigned short int)strtoul(value_str, NULL, 2);// If negative, handle two's complement and convert to decimal
       double decimal_value = -(65536.0 - raw_value);// Apply two's complement conversion
       return decimal_value ; // Adjust the divisor based on the number of decimal places
   } else {
       double decimal_value = strtoul(value_str, NULL, 2);// If positive, convert directly to decimal
       return decimal_value ; // Adjust the divisor based on the number of decimal places
   }
}
char* map_sensor_type(char* binary_sensor_type, int size, char* board_type) {
   int sensor_type_index = strtol(binary_sensor_type, NULL, 2); // Base 2 for binary
   char* sensor_type = (char*)malloc(10 * sizeof(char)); // Allocate enough space for "ISFET" + digit + null terminator
   if (sensor_type == NULL) {
       printf("Memory allocation failed\n");
       return NULL;
   }
   if (strcmp(board_type, "0") == 0) { // IMB board
          if (sensor_type_index >= 0 && sensor_type_index <= 6) {
              snprintf(sensor_type, 10, "ISFET%d", sensor_type_index+1);
          } else if (sensor_type_index == 7) {
              strcpy(sensor_type, "AMP");
          } else if (sensor_type_index == 8) {
              strcpy(sensor_type, "ORP");
          } else {
              printf("Unknown sensor type for IMB: %s\n", binary_sensor_type);
              free(sensor_type);
              return NULL;
       }
   } else if (strcmp(board_type, "1") == 0) { // iAQUA board
       if (sensor_type_index >= 1 && sensor_type_index <= 9) { // Adjusted range for iAQUA
           snprintf(sensor_type, 10, "ISFET%d", sensor_type_index);
       } else if (sensor_type_index == 10) {
           strcpy(sensor_type, "AMP");
       } else if (sensor_type_index == 11) {
           strcpy(sensor_type, "ORP");
       } else {printf("Unknown sensor type for iAQUA: %s\n", binary_sensor_type);
           free(sensor_type);
           return NULL;
       }
   } else {
       printf("Unknown board type: %s\n", board_type);
       free(sensor_type);
       return NULL;
   }
   return sensor_type;
}
SensorData* extract_sensor_data(char* line, args_displayInfo* params) {
   if (!line) {
       printf("NO LINE");
       return NULL;
   }
   SensorData* data = (SensorData*)malloc(sizeof(SensorData));
   if (data == NULL) {
       printf("Failed to allocate memory for SensorData\n");
       return NULL;
   }
   data->board_type[0] = line[40];
   data->board_type[1] = '\0';
   data->timestamp = binary_to_decimal_time(line, 32);
   //uint8_t chip = binary_to_decimal(line, 32, 39);
   data->chip_address=binary_to_decimal(line, 32, 39);
   data->value = binary_to_decimal_sensor_data(line + 48, 16);
   char sensor_type_bits[8];
   strncpy(sensor_type_bits, line + 41, 7);
   sensor_type_bits[7] = '\0';
   data->sensor_type = map_sensor_type(sensor_type_bits, 7, data->board_type);

   if (data->sensor_type == NULL) {
       free(data);
       return NULL;
   }
   if (strcmp(data->sensor_type, "AMP") == 0) {
       strcpy(data->units, "mA");
   } else {
       strcpy(data->units, "mV");
   }
   return data;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////MQTT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void publishing(SensorData* data, MQTTClient client, char* topic) {//////////////////Function to publish stuff
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   // Determine the board type string
   const char* board_type_str;
   if (data->board_type[0]=='0'){
      board_type_str="IMB";
   }
   else{board_type_str="iAQUA";}
   if (new_data) {
	   // Add data to the JSON object
       struct json_object *json_obj = json_object_new_object();
       json_object_object_add(json_obj, "Chip", json_object_new_int(data->chip_address));
       json_object_object_add(json_obj, "Sensor", json_object_new_string(data->sensor_type));
       json_object_object_add(json_obj, "Value", json_object_new_double(data->value));
       json_object_object_add(json_obj, "Units", json_object_new_string(data->units));
       json_object_object_add(json_obj, "Timestamp", json_object_new_int(data->timestamp));
       json_object_object_add(json_obj, "Board Type", json_object_new_string(board_type_str));

       const char *json_str = json_object_to_json_string_ext(json_obj, JSON_C_TO_STRING_PRETTY);// Convert JSON object to string
       pubmsg.payload = (void*)json_str;// Ensure the payload is the JSON string
       pubmsg.payloadlen = strlen(json_str);
       new_data = false;
       pubmsg.qos = QOS;
       pubmsg.retained = 0;

       MQTTClient_publishMessage(client, topic, &pubmsg, &token);
       MQTTClient_waitForCompletion(client, token, TIMEOUT);
       json_object_put(json_obj);// Free JSON object after publishing
   }
}
////////For subscribing and getting date
void delivered(void *context, MQTTClient_deliveryToken dt) {
   printf("Message with token value %d delivery confirmed\n", dt);
   deliveredtoken = dt;
}
int msgarrvd(void *context, char *topic2, int topicLen, MQTTClient_message *message) {
   snprintf(last_message, sizeof(last_message), "%.*s", message->payloadlen, (char*)message->payload);
   printf("Received message: %s\n", last_message);
   MQTTClient_freeMessage(&message);
   MQTTClient_free(topic2);
   return 1;
}
void connlost(void *context, char *cause) {
   printf("Connection lost due to: %s\n", cause);
}
void create_file(const char* filepath2, const char* filename2,char* fullpath) {
   snprintf(fullpath, 512, "%s/%s.txt", filepath2, filename2);
   struct stat st = {0};
   if (stat(filepath2, &st) == -1) {
       printf("Directory does not exist: %s\n", filepath2);
       return;
   }
   FILE* file = fopen(fullpath, "w");
   if (file == NULL) {
       perror("Failed to create file");
       printf("File path: %s\n", fullpath);
       return;
   }
   fprintf(file, "This is the file for message: %s\n", filename2);
   fclose(file);
   printf("File created: %s\n", fullpath);
}
void cleanup_filename(char* str) {
   char* i = str;
   char* j = str;
   while (*j != 0) {
       if (*j == ' ' || *j == ':') {
           *i = '-';
       } else {
           *i = *j;
       }
       i++;
       j++;
   }
   *i = 0;
}
