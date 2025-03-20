#ifndef DEFINE_BUFFER_H
#define DEFINE_BUFFER_H
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "MQTTClient.h"
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include <pthread.h>
#include "json.h"
#include "json_inttypes.h"
#include "json_object.h"
#include "json_tokener.h"
#include "../gpio_configuration/gpio_configuration.h"
#include "../i2c_custom_C/I2C_AER.h"

#define ADDRESS     "tcp://192.168.4.22:1883"
#define CLIENTID    "Main"
#define TOPIC       "MQTTExamples"
#define QOS         0
#define TIMEOUT     10000L

#define BUFFER_SIZE 86
extern bool new_data;
extern volatile MQTTClient_deliveryToken deliveredtoken;
extern char last_message[256];
extern char* file_path;

// Structure for current interpreted sensor data
typedef struct {
    uint8_t chip_address;
    char* sensor_type;
    uint32_t timestamp;
    int16_t value;
    char units[4];
    char board_type[1];
} SensorData;
typedef struct {
    char sensor_data_i[64]; // Store interpreted binary data line
    } SensorBufferSlot;
//gain and offset IMB
typedef struct {
    double gain;
    double offset;
} GainOffset;
// for look_up tab
typedef struct struct_info struct_info;
struct struct_info{
	uint8_t address_chip_tab[77];
	uint8_t sensor_tab[77];
	uint8_t prio_tab[77];
};
//to get gain and offset iAQUA
typedef struct {
    double a;
    double b;
} Calibration;
typedef struct {
    uint16_t sensor;
    int16_t data;
    uint32_t time;
}vector_imb;
// Declare global mutex and condition variable
extern pthread_mutex_t buffer_lock;
extern pthread_cond_t buffer_cond;

// Hardcoded Gain and Offset Values for sensor_gains_offsets (IMB Board)**
static const GainOffset HARD_CODED_SENSOR_GAINS_OFFSETS[9] = {
    {6700.8, 3.8},   // Sensor 0
    {6641.7, 6.6},   // Sensor 1
    {6771.5, -5.2},  // Sensor 2
    {6790.8, 282.6}, // Sensor 3
	{6462.54, -149.55}, // Sensor 4
    {6686, -94.8},  // Sensor 5
    {6641.4, 30.8},  // Sensor 6
    {1.0,0.0},  // Sensor 7 (AMPEROMETRIC)
    {13281.9788, 19.1990}       // Sensor 8 (ORP)
};
// Hardcoded Gain Offset Matrix for iAQUA Board**
static const Calibration HARD_CODED_GAIN_OFFSET_MATRIX[128][11][2] = {
		    [119] = {  // Chip 119
		        [1] = { {0.7880, -3.2000}, {0.9040, -9.2000} },  // ISFET1 (positive, negative)
		        [2] = { {0.8400, 3.8000}, {0.9680, -7.6000} },   // ISFET2
		        [3] = { {0.6000, -2.6000}, {0.6200, -5.2000} },  // ISFET3
		        [4] = { {0.5160, -9.2000}, {0.4520, -10.0000} }, // ISFET4
		        [5] = { {0.8760, -8.2000}, {0.9400, -19.0000} }, // ISFET5
		        [6] = { {0.4920, -2.6000}, {0.4360, -2.8000} },  // ISFET6
		        [7] = { {0.7000, -10.6000}, {0.6640, -13.8000} },// ISFET7
		        [8] = { {0.6440, -8.2000}, {0.6360, -10.0000} }  // ISFET8
		    }
		};

typedef struct _args_displayInfo {
    SensorBufferSlot* sensor_buffer;//initialice the buffer
    SensorBufferSlot** pointread;// pointer buffer read
    SensorBufferSlot** pointwrite;// pointer buffer write
    MQTTClient client;
    GainOffset sensor_gains_offsets[9];
    Calibration gain_offset_matrix[128][11][2];  // [chip_address][sensor][0 for positive, 1 for negative]
    FILE* file2;
    uint8_t address_chip_TSMC[7];
    uint32_t program_start_time;// to get start time in uint32
    long long start_program; //to get start time in long long
    vector_imb current_value_imb[9][3];  // 9 sensors on IMB board
    int16_t value_matrix[77][4];           // Value vector for iAQUA sum always
    int16_t value_matrix2[77][4]; //value vector before reset
    uint8_t address_adc_BUSB[7];
    uint8_t address_adc_BUSA[2];
    gpio_config_t gpio_config;
    long long last_reset_time_iaqua;
    long long last_reset_time_imb;
    pthread_mutex_t matrix1_lock;// write directly from the iaqua thread
    pthread_mutex_t imb_lock;// write directly from the iaqua thread
    struct_info lookup_tab; // data of sensors iAQUA, chip address, sensor type and priority
} args_displayInfo;

/////////Function prototypes
void publishing(SensorData* data, MQTTClient client, char* topic);
SensorData* extract_sensor_data(char* line, args_displayInfo* params);
long long binary_to_decimal(char* binary_str, int start, int end);
double binary_to_decimal_sensor_data(char* binary_data, int size);
long long current_timestamp();
void delivered(void *context, MQTTClient_deliveryToken dt);
int msgarrvd(void *context, char *topic2, int topicLen, MQTTClient_message *message);
void connlost(void *context, char *cause);
void cleanup_filename(char* str);
void create_file(const char* filepath2, const char* filename2,char* fullpath);
void* write_data_thread(void *args);
void* buffer_read_thread(void *args);
void* timer_gen_thread(void *args);
void* sensor_iaqua(void* args);
void* sensor_imb(void* args);
void initialize_i2c_custom(args_displayInfo* params);
void initialize_i2c_standard(args_displayInfo* params);
void setup_mqtt(args_displayInfo* params);
void reset_value_matrix(args_displayInfo* params);
void build_binary_string(char* binary_string, uint32_t timestamp, uint8_t chip_address, uint8_t board_type,int sensor_type_index, int data);
void calculate_checksum(const char* line_i, char* checksum);
#endif // DEFINE_BUFFER_H
