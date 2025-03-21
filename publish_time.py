import paho.mqtt.client as mqtt
import datetime
import time

# Define the MQTT broker address and topic
broker_address = "192.168.4.22"  # Use your local IP address here
topic = "MQTTExamples/Date"
Client_ID = "Date"

# Callback function to handle connection to MQTT broker
def on_connect(client, userdata, flags, rc):
    
    
    
    if rc == 0:
        print("Connected to MQTT broker")
        # Publish the current date and time immediately after connecting
        publish_current_time(client)
        client.subscribe(topic)
    else:
        print(f"Failed to connect to MQTT broker, return code: {rc}")


# Function to publish the current date and time
def publish_current_time(client):
    current_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print("Publishing current time:", current_time)
    client.publish(topic, current_time)

# Create MQTT client instance
client = mqtt.Client(client_id=Client_ID, clean_session=True)
client.on_connect = on_connect


# Connect to MQTT broker
client.connect(broker_address)


# Start the MQTT client loop in a separate thread

client.loop_start()

try:
    while True:
        # Publish current time every 10 seconds
        publish_current_time(client)
        time.sleep(10)
except KeyboardInterrupt:
    print("Disconnecting from broker")
    client.disconnect()
    client.loop_stop() 
    
