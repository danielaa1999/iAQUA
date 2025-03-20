/**
*
* @file my_i2c.c
*
*
** MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- -----------------------------------------------
* 1.00  jcm  01/07/22 First release
* 2.00  psl  10/07/23 adding new functions
******************************************************************************/
/***************************** Include Files *********************************/
#include "I2C_AER.h"
#include "../mqtt/define_buffer.h"
/************************** Function Prototypes ******************************/
void Config_I2C(uint8_t rw, uint8_t chip_addr, uint8_t sensor_addr, uint8_t reg_addr, uint8_t write_data);
uint8_t Is_Busy();
void Clear_ON();
void Clear_OFF();
void Clear();
void ACK_test_ON();
void ACK_test_OFF();
void Enable_ON();
void Enable_OFF();
void Event_ON();
void Event_OFF();
/**************************** Global Variables ********************************/
uint32_t reg1_value ;
uint32_t reg2_value;
uint32_t reg3_value;
static uint32_t i2c_clk_div = (F_CLK/F_I2C-8)/4+1;
typedef uint32_t u32;
volatile u32 *i2c_vptr;
volatile uint8_t AMP_state;
volatile uint32_t *clk_vptr;
int i2c_fd = -1, clk_fd = -1;
#define UIO_DEVICE "/dev/uio0"  // Path to the UIO device of I2C_Master
#define UIO_CLK_DEVICE "/dev/uio1"         // UIO device for clock divider
#define AXI_SIZE 0x10000
static int uio_fd = -1;  // File descriptor for UIO device
const uint8_t tab_sensor[11]={ISFET1, ISFET2,ISFET3, ISFET4, ISFET5, ISFET6,ISFET7,ISFET8,ISFET9,ORP,AMP3};/* address type of sensor table */
/************************** Function Declarations ******************************/
void Init_I2C() {
    // Open UIO for I2C Master
    uio_fd = open(UIO_DEVICE, O_RDWR);
    if (uio_fd < 0) {
        perror("Failed to open /dev/uio0 for I2C Master");
        exit(EXIT_FAILURE);
    }

    // Map I2C register space
    i2c_vptr = (volatile uint32_t *)mmap(NULL, AXI_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, uio_fd, 0);
    if (i2c_vptr == MAP_FAILED) {
        perror("Failed to mmap UIO device for I2C Master");
        close(uio_fd);
        exit(EXIT_FAILURE);
    }

    // Open UIO for Clock Divider
    clk_fd = open(UIO_CLK_DEVICE, O_RDWR);
    if (clk_fd < 0) {
        perror("Failed to open /dev/uio1 for Clock Divider");
        close(uio_fd);
        exit(EXIT_FAILURE);
    }

    // Map Clock Divider register space
    clk_vptr = (volatile uint32_t *)mmap(NULL, AXI_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, clk_fd, 0);
    if (clk_vptr == MAP_FAILED) {
        perror("Failed to mmap UIO device for Clock Divider");
        close(uio_fd);
        close(clk_fd);
        exit(EXIT_FAILURE);
    }

    // Set clock divider
    clk_vptr[CLK_DIVIDER_OFFSET / sizeof(uint32_t)] = i2c_clk_div;

    // Configure the I2C Master
    reg2_value = (i2c_clk_div << CLK_DIVIDER_OFFSET);
    i2c_vptr[(I2C_MASTER_REG2_ADDR - I2C_MASTER_BASE_ADDR) / sizeof(uint32_t)] = reg2_value;
}

void Cleanup_I2C() {
    if (i2c_vptr != MAP_FAILED) munmap((void *)i2c_vptr, AXI_SIZE);
    if (clk_vptr != MAP_FAILED) munmap((void *)clk_vptr, AXI_SIZE);
    if (uio_fd >= 0) close(uio_fd);
    if (clk_fd >= 0) close(clk_fd);
}
void Config_I2C(uint8_t rw, uint8_t chip_addr, uint8_t sensor_addr, uint8_t reg_addr, uint8_t write_data) {
    // Ensure the clock divider is set in the clock control register using uio1
    if (clk_vptr != MAP_FAILED) {
        clk_vptr[CLK_DIVIDER_OFFSET / sizeof(uint32_t)] = i2c_clk_div;
       // printf("Clock divider set to: %u at offset %u\n", i2c_clk_div, CLK_DIVIDER_OFFSET);
    } else {
        fprintf(stderr, "Clock divider pointer (clk_vptr) is invalid.\n");
        return;
    }

    // Set up the I2C configuration in Reg1 using uio0
    reg1_value = ((rw << RW_OFFSET) |
                  (write_data << WRITE_DATA_OFFSET) |
                  (((sensor_addr << 3) | reg_addr) << REG_ADDR_OFFSET) |
                  (chip_addr << DEVICE_ADDR_OFFSET));

    // Calculate the correct offset for Reg1 in the I2C master register space
    uint32_t reg1_offset = (I2C_MASTER_REG1_ADDR - I2C_MASTER_BASE_ADDR) / sizeof(uint32_t);

    // Access the I2C configuration register using the mapped pointer
    if (i2c_vptr != MAP_FAILED) {
        i2c_vptr[reg1_offset] = reg1_value;
    } else {
        fprintf(stderr, "I2C pointer (i2c_vptr) is invalid.\n");
        return;
    }
}
void Set_Clock_Divider(uint32_t divider) {
    static uint32_t last_divider = 0;  // Cache the last written value
    if (divider != last_divider) {
        clk_vptr[CLK_DIVIDER_OFFSET / sizeof(uint32_t)] = divider;
        last_divider = divider;
    }
}
uint8_t Is_Busy() {
    uint32_t reg3_offset = (I2C_MASTER_REG3_ADDR - I2C_MASTER_BASE_ADDR) / sizeof(uint32_t);
    uint32_t reg3_value = i2c_vptr[reg3_offset];
    return (reg3_value >> BUSY_OFFSET) & BUSY_MASK;
}
void ACK_test_ON() {
	//clk_vptr[CLK_DIVIDER_OFFSET / sizeof(uint32_t)] = i2c_clk_div;
    uint32_t reg0_offset = (I2C_MASTER_REG0_ADDR - I2C_MASTER_BASE_ADDR) / sizeof(uint32_t);// Calculate the correct offset for I2C_MASTER_REG0_ADDR
    i2c_vptr[reg0_offset] = (1 << ACK_TEST_OFFSET);// Set the ACK_TEST bit in I2C Master Register 0
    return;
}
void ACK_test_OFF() {
	//clk_vptr[CLK_DIVIDER_OFFSET / sizeof(uint32_t)] = i2c_clk_div;
    uint32_t reg0_offset = (I2C_MASTER_REG0_ADDR - I2C_MASTER_BASE_ADDR) / sizeof(uint32_t);// Calculate the correct offset for I2C_MASTER_REG0_ADDR
    i2c_vptr[reg0_offset] = (0 << ACK_TEST_OFFSET);// Clear the ACK_TEST bit by writing 0 to it
    return;
}
void Clear_ON() {
	//clk_vptr[CLK_DIVIDER_OFFSET / sizeof(uint32_t)] = i2c_clk_div;
    uint32_t reg0_offset = (I2C_MASTER_REG0_ADDR - I2C_MASTER_BASE_ADDR) / sizeof(uint32_t);
    i2c_vptr[reg0_offset] |= (1 << CLEAR_OFFSET);
    return;
}

void Clear_OFF() {
	//clk_vptr[CLK_DIVIDER_OFFSET / sizeof(uint32_t)] = i2c_clk_div;
    uint32_t reg0_offset = (I2C_MASTER_REG0_ADDR - I2C_MASTER_BASE_ADDR) / sizeof(uint32_t);
    i2c_vptr[reg0_offset] &= ~(1 << CLEAR_OFFSET);
    return;
}
void Enable_ON() {
	//clk_vptr[CLK_DIVIDER_OFFSET / sizeof(uint32_t)] = i2c_clk_div;
    uint32_t reg0_offset = (I2C_MASTER_REG0_ADDR - I2C_MASTER_BASE_ADDR) / sizeof(uint32_t);
    i2c_vptr[reg0_offset] |= (1 << ENABLE_OFFSET);
    return;
}
void Enable_OFF() {
	//clk_vptr[CLK_DIVIDER_OFFSET / sizeof(uint32_t)] = i2c_clk_div;
    uint32_t reg0_offset = (I2C_MASTER_REG0_ADDR - I2C_MASTER_BASE_ADDR) / sizeof(uint32_t);
    i2c_vptr[reg0_offset] &= ~(1 << ENABLE_OFFSET);
    return;
}
void Event_ON() {
	//clk_vptr[CLK_DIVIDER_OFFSET / sizeof(uint32_t)] = i2c_clk_div;
    uint32_t reg0_offset = (I2C_MASTER_REG0_ADDR - I2C_MASTER_BASE_ADDR) / sizeof(uint32_t);
    i2c_vptr[reg0_offset] |= (1 << ENABLE_EVENT_OFFSET);
    return;
}
void Event_OFF() {
	//clk_vptr[CLK_DIVIDER_OFFSET / sizeof(uint32_t)] = i2c_clk_div;
    uint32_t reg0_offset = (I2C_MASTER_REG0_ADDR - I2C_MASTER_BASE_ADDR) / sizeof(uint32_t);
    i2c_vptr[reg0_offset] &= ~(1 << ENABLE_EVENT_OFFSET);
    return;
}
void Clear() {
    while (Is_Busy());// Wait for the I2C to not be busy
    Clear_ON();// Set the CLEAR bit
    while (!Is_Busy());// Wait for the I2C to be busy
    Clear_OFF();// Clear the CLEAR bit
    while (Is_Busy()); // Wait for the I2C to finish the clear operation
}
uint8_t Check_ACK_I2C(uint8_t chip_addr)
{
    // Ensure I2C is idle
    while (Is_Busy());
    // Configure I2C for the ACK test
    Config_I2C(0, chip_addr, 0, 0, 0);
    // Wait for configuration to be applied
    while (Is_Busy());
    // Perform ACK test
    ACK_test_ON();
    // Wait for the ACK test to begin
    while (!Is_Busy());
    // End ACK test
    ACK_test_OFF();
    // Wait for the ACK test to finish
    while (Is_Busy());
    // Read the ACK result
    uint32_t reg3_offset = (I2C_MASTER_REG3_ADDR - I2C_MASTER_BASE_ADDR) / sizeof(uint32_t);
    uint32_t reg3_value = i2c_vptr[reg3_offset];
    __sync_synchronize(); // Ensure memory synchronization
    uint8_t ack = (reg3_value >> ACK_OFFSET) & ACK_MASK;
    // Clear I2C state
    Clear();
    return ack;
}
uint8_t Read_I2C(uint8_t chip_addr, uint8_t sensor_addr, uint8_t reg_addr) {
    Config_I2C(1, chip_addr, sensor_addr, reg_addr, 0);
    while (Is_Busy());// Wait for not busy
    Enable_ON();
    while (!Is_Busy());// Wait for busy
    Enable_OFF();
    while (Is_Busy());// Wait to finish the I2C read
    // Validate i2c_vptr and offset
    if (i2c_vptr == NULL || i2c_vptr == MAP_FAILED) {
        fprintf(stderr, "Error: i2c_vptr is invalid.\n");
        return 0xFF; // Error code
    }
    uint32_t reg3_offset = I2C_MASTER_REG3_ADDR / sizeof(uint32_t);
    if (reg3_offset * sizeof(uint32_t) >= AXI_SIZE) {
        fprintf(stderr, "Error: reg3_offset exceeds mapped region.\n");
        return 0xFF; // Error code
    }
    // Read and process the data
    uint32_t reg3_value = i2c_vptr[reg3_offset];
    uint8_t miso_data = (reg3_value >> MISO_DATA_OFFSET) & MISO_DATA_MASK;
    printf("reg3_value: 0x%x, MISO data: 0x%x\n", reg3_value, miso_data);
    return miso_data;
}
uint16_t Event_I2C() {
    // Null pointer check
    if (i2c_vptr == NULL) {
        fprintf(stderr, "Error: i2c_vptr is NULL\n");
        return 0xFFFF;  // Error code
    }
    // Wait for not busy
    while (Is_Busy());
    // Trigger event
    Event_ON();
    // Wait for busy
    while (!Is_Busy());
    // Clear event
    Event_OFF();
    // Wait to finish the event
    while (Is_Busy());
    // Read and return MISO data (with synchronization)
    uint32_t reg3_value = i2c_vptr[(I2C_MASTER_REG3_ADDR - I2C_MASTER_BASE_ADDR) / sizeof(uint32_t)];
    return (uint16_t)(reg3_value >> MISO_DATA_OFFSET);
}
/*****************************************
   	 FUNCTION void write_i2c(uint8_t address_chip, uint8_t address_sensor, uint8_t reg, uint8_t reg_data)

   	 1) Purpose and functionality : function following the I2C - AER protocol, it writes the custom writing protocol
     2) Preconditions and limitations : need the address of the chip, the address or the sensor, the register and the data
     3) Status label: PASSED TEST
******************************************/
void write_i2c(uint8_t address_chip, uint8_t address_sensor, uint8_t reg, uint8_t reg_data){
	Config_I2C(0, address_chip, address_sensor, reg, reg_data);
	while (Is_Busy());// Wait for not busy
	Enable_ON();
	while (!Is_Busy());// Wait for busy
	Enable_OFF();
	while (Is_Busy());// Wait to finish the I2C write
	return;
}
/*****************************************
   	 FUNCTION void init_sensor(uint8_t address_chip, int start_j, int end_j)

   	 1) Purpose and functionality : function initializes sensor with the register A in order to start of measurement
     2) Preconditions and limitations : need to know how to the sensor perform a measurement
     3) Status label: PASSED TEST
     *Note init_sensor2 is for only writing in register A
******************************************/
void init_sensor(uint8_t address_chip, int start_j, int end_j) {
    int i, j;
    uint8_t regA_val[4] = {0x40,0xC0,0xE0, 0xF0};
    uint8_t regC_val = 0x30;
    uint8_t regD_val = 0x00;
    for (i = 0; i < 9; i++) {
    	if ((address_chip == 119 && i == 8) ) {
    	           continue;
    	       }
        // Iterate `j` from `start_j` to `end_j` inclusively
        for (j = start_j; j <= end_j; j++) {
            write_i2c(address_chip, tab_sensor[i], regA, regA_val[j]);
        }
        // Perform the additional writes
        write_i2c(address_chip, tab_sensor[i], regC, regC_val);
        write_i2c(address_chip, tab_sensor[i], regD, regD_val);
    }
}
void init_sensor2(uint8_t address_chip, int start_j, int end_j) {
    int i, j;
    uint8_t regA_val[4] = {0x40,0xC0,0xE0, 0xF0};
    for (i = 0; i < 9; i++) {
    	if ((address_chip == 119 && i == 8) ) {
    	           continue;
    	       }
        // Iterate `j` from `start_j` to `end_j` inclusively
        for (j = start_j; j <= end_j; j++) {
            write_i2c(address_chip, tab_sensor[i], regA, regA_val[j]);
        }
    }
}
/*****************************************
   	 FUNCTION void init_ORP_AMP(void)

   	 1) Purpose and functionality : function enable the AMP and ORP
     2) Status label: Tested and works
******************************************/
void init_ORP_AMP(uint8_t address_chip) {
    uint8_t regA_val[4] = {0x40, 0x42, 0x40, 0xE0};
    uint8_t regC_val = 0xFF;
    uint8_t regD_val = 0xFC;
    uint8_t regE_val = 0x00;
    uint8_t sensors[] = {ORP, AMP3};
    for (int s = 0; s < 2; s++) {  // Loop through ORP and AMP
        uint8_t sensor_type = sensors[s];
        for (int j = 0; j < 4; j++) {
            write_i2c(address_chip, sensor_type, regA, regA_val[j]);
        }
        write_i2c(address_chip, sensor_type, regC, regC_val);
        write_i2c(address_chip, sensor_type, regD, regD_val);
        write_i2c(address_chip, sensor_type, regE, regE_val);
    }
    AMP_state = 0;
}
/*****************************************
   	 FUNCTION struct_info init_PRIO(uint8_t address_chip)

   	 1) Purpose and functionality : function gives every sensor a priority by returning a structure
     2) Preconditions and limitations : need to know how to the register B works
     3) Status label: PASSED TEST for multiple chips
******************************************/
void init_PRIO(args_displayInfo* params, uint8_t address_chip, int start_index) {
    int end_index = start_index + 10;
    uint8_t regB_val = start_index;
    // Fill the structure for the specified range of sensors
    for (int i = start_index; i <= end_index; i++) {
        write_i2c(address_chip, tab_sensor[i % 11], regB, regB_val);
        params->lookup_tab.prio_tab[i] = regB_val;
        params->lookup_tab.address_chip_tab[i] = address_chip;
        params->lookup_tab.sensor_tab[i] = tab_sensor[i % 11];
        regB_val++;
    }
}
/*****************************************
   	 FUNCTION void disable_sensor(uint8_t chip_address)

   	 1) Purpose and functionality : disables the sensors before running the code
     2) Preconditions and limitations : need to know how to the register A works
     3) Status label: PASSED TEST for multiple chips
     *Note:Disable_sensor2 is for disabling a sensor that is giving error,
******************************************/
void disable_sensor(uint8_t chip_address){//Disable all sensors in the begining of the code
	 for (int i = 0; i < 11; i++) {
	write_i2c(chip_address, tab_sensor[i], regA,0x00);
}
}
void disable_sensor2(uint8_t chip_address, uint8_t sensor_index) {// Disable only the specified sensor
    write_i2c(chip_address, tab_sensor[sensor_index], regA, 0x00);
}
