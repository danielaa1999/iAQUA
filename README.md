the codes that run on the linux OS of the Artyz7

code0.c: main code, it gets the data from the sensor and sends it to the buffer. It is running three thread one for timing, another to read imb data and the last one to read iaqua data.

mqtt.c: buffer read and publish the data through mqtt. It is running two threads one to write the data into the buffer another to read it and publish it.

ADS1100_configuration: get the adc data for each imb sensor.

DAC8574_configuration: Configure the DACs of IMB and iAQUA pcbs.

gpio_configuration: Configure the imb GPIOs in order to turn ON and OFF the circuit of each sensor.

my_periph: Configure and read the GPIO used as the SRQ line for the I2C custom of the iAQUA sensors.

i2c_custom_C:Read the event data though an uio in the linux OS iwth the help of another uio to provide the timing
