import json
import csv
import time
import paho.mqtt.client as mqtt
import matplotlib
matplotlib.use('SVG')
import matplotlib.pyplot as plt
import threading
from queue import Queue
from PyQt5 import QtWidgets, QtCore
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
import sys
# Sliding window settings
window_size = 400
window_step = 50#860ms in minutes
start_time = int(time.time())
current_window_start = 0
window_end = 0
last_reset=0
# Define the MQTT broker address and topi
broker_address = "192.168.4.22"  # Replace with your broker IP address
topic = "MQTTExamples"
Client_ID = "GRAPHS"
# Data dictionary to hold sensor data
sensor_data = {"iAQUA": [], "IMB": []}
# Buffered data for CSV writing
buffered_data = {"iAQUA": [], "IMB": []}
###################################################################################################################
#########Connect to mqtt
###################################################################################################################
# Define the on_connect callback function
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker")
        client.subscribe(topic)
    else:
        print(f"Failed to connect to MQTT broker, return code: {rc}")
# Callback function to handle MQTT message reception
def on_message(client, userdata, message):
    payload = message.payload.decode('utf-8')
    print(f"Received message on topic '{message.topic}': {payload}")
    # Parse the JSON payload
    try:
        payload_json = json.loads(payload)
        process_message(payload_json)
    except json.JSONDecodeError as e:
        print(f"Failed to decode JSON: {e}")
# MQTT client setup and connection
def connect(broker_address, topic):
    client = mqtt.Client(client_id=Client_ID, clean_session=True)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(broker_address)
    print("Data connection completed.")
    return client
###################################################################################################################
#########Save to csv
###################################################################################################################
# Process the received JSON message
def process_message(payload):
    chip = payload.get("Chip")
    sensor = payload.get("Sensor")
    value = payload.get("Value")
    timestamp = payload.get("Timestamp")
    board_type = payload.get("Board Type", "no_board_type")  # Default to "no_board_type" if not provided
    print(f"Processing message: Chip={chip}, Sensor={sensor}, Value={value}, Timestamp={timestamp}, Board type={board_type}")
    # Check if all required data is available
    if chip is not None and value is not None and timestamp is not None:
        if board_type in sensor_data:
            # Add actual timestamp data to sensor_data
            sensor_data[board_type].append((timestamp, chip, sensor, value, None))
            buffered_data[board_type].append((timestamp, chip, sensor, value, None))
        else:
            print(f"Unknown board type: {board_type}")
    else:
        print("Incomplete data received, skipping")

# Function to save sensor data to CSV files periodically
def save_to_csv():
    while True:
        time.sleep(5)  # Adjust the interval as needed
        for board_type, data in buffered_data.items():
            if data:
                file_name = f"{board_type}_sensor_data.csv"
                with open(file_name, mode="a", newline="") as csvfile:
                    writer = csv.writer(csvfile)
                    # Write sensor data rows
                    for entry in data:
                        writer.writerow(entry)
                buffered_data[board_type] = []  # Clear the buffer after writing

###################################################################################################################
#########Graphs
###################################################################################################################
# Plotting Canvas
class MplCanvas(FigureCanvas):
    def __init__(self, parent=None):
        self.fig, self.ax = plt.subplots()
        super().__init__(self.fig)
        self.ax.set_xlim(0, window_size)
        self.ax.set_ylim(-2000, 2000)
    def plot_iAQUA_chip_data(self, chip_address, graph_data, window_start, window_end):
            print(f"Plotting iAQUA Chip {chip_address} data...")
            self.ax.clear()
            self.ax.set_title(f"Sensor Data: iAQUA Chip {chip_address}")
            self.ax.set_xlim(window_start, window_end)  # Set the x-axis limits to the window range
            self.ax.set_ylim(-200, 200)

            # Filter the data for this chip within the sliding window
            iAQUA_chip_data = [entry for entry in graph_data["iAQUA"] if entry[1] == chip_address]
            if iAQUA_chip_data:
                sensors = ["ISFET1", "ISFET2", "ISFET3", "ISFET4", "ISFET5", "ISFET6", "ISFET7", "ISFET8", "ISFET9", "ORP", "AMP"]
                for sensor in sensors:
                    # Filter the sensor data based on window_start and window_end
                    sensor_data = [(entry[0], entry[3]) for entry in iAQUA_chip_data if entry[2] == sensor]
                    if sensor_data:
                        timestamps, values = zip(*sensor_data)
                        timestamps = [t / 1000 for t in timestamps]  # Adjust timestamp to seconds
                        self.ax.plot(timestamps, values, label=sensor)  # Plot the sensor data
            else:
                print("No data available for this chip")

            if self.ax.get_lines():
                self.ax.legend(loc='upper right')
                self.ax.set_xlabel('Time (seconds)')
                self.ax.set_ylabel('Value (mV)')
                self.draw()

    def plot_IMB_data(self, graph_data, window_start, window_end):
        print("Plotting IMB data...")
        self.ax.clear()  # Clear previous plots
        self.ax.set_title("Sensor Data: IMB")
        self.ax.set_xlim(window_start, window_end)  # Set the x-axis limits to the window range
        self.ax.set_ylim(-2000, 2000)
        # Group and plot data for each sensor type
        sensors = ["ISFET1", "ISFET2", "ISFET3", "ISFET4", "ISFET5", "ISFET6", "ISFET7", "ORP", "AMP"]
        for sensor in sensors:
            # Filter the sensor data based on window_start and window_end
            sensor_data = [(entry[0], entry[3]) for entry in graph_data.get("IMB", []) if entry[2] == sensor]
            if sensor_data:
                timestamps, values = zip(*sensor_data)
                timestamps = [t / 1000 for t in timestamps]  # Adjust timestamp to minutes
                self.ax.plot(timestamps, values, label=sensor)  # Plot the sensor data

        if self.ax.get_lines():
            self.ax.legend(loc='upper right')  # Add legend
            self.ax.set_xlabel('Time (minutes)')
            self.ax.set_ylabel('Value (mV)')
            self.draw()


def plot_windows():
    global main_windows, tab_widget
    main_windows = {}
    app = QtWidgets.QApplication(sys.argv)

    # Create the main window
    main_window = QtWidgets.QMainWindow()
    main_window.setWindowTitle("Sensor Data Viewer")

    # Create a tab widget to hold multiple canvases
    tab_widget = QtWidgets.QTabWidget()
    main_window.setCentralWidget(tab_widget)

    # Dynamically create tabs for iAQUA chips based on received data
    unique_chips = sorted(set(entry[1] for entry in sensor_data["iAQUA"]))  # Get all unique chip numbers
    for chip in unique_chips:
        mpl_canvas_iaqua = MplCanvas()
        tab_widget.addTab(mpl_canvas_iaqua, f"iAQUA Chip {chip}")
        main_windows[f"iAQUA_{chip}"] = mpl_canvas_iaqua

    # Create a tab for IMB data
    mpl_canvas_imb = MplCanvas()
    tab_widget.addTab(mpl_canvas_imb, "IMB Data")
    main_windows["IMB"] = mpl_canvas_imb

    main_window.show()

    # Set up a timer to periodically update the graphs
    timer = QtCore.QTimer()
    timer.timeout.connect(update_graphs)
    timer.start(800)  # Update every 800 milliseconds
    sys.exit(app.exec_())


# Update graphs
def update_graphs():
    global start_time, current_window_start, last_reset, main_windows
    elapsed_time = time.time() - start_time
    current_graph_time = elapsed_time  # Convert to seconds

    if current_graph_time >= window_size:
        if (current_graph_time - last_reset) >= window_step:
            current_window_start += window_step
            last_reset = current_graph_time

    window_end = current_window_start + window_size

    # Check for new unique iAQUA chips and add tabs dynamically
    unique_chips = sorted(set(entry[1] for entry in sensor_data["iAQUA"]))  # All unique chip numbers
    for chip in unique_chips:
        tab_name = f"iAQUA_{chip}"
        if tab_name not in main_windows:
            print(f"Adding new tab for iAQUA Chip {chip}")
            mpl_canvas_iaqua = MplCanvas()
            tab_widget.addTab(mpl_canvas_iaqua, f"iAQUA Chip {chip}")
            main_windows[tab_name] = mpl_canvas_iaqua

    # Update existing tabs
    for key, canvas in main_windows.items():
        try:
            if "iAQUA" in key:
                chip_number = int(key.split("_")[1])  # Extract chip number from tab name
                chip_data = [entry for entry in sensor_data["iAQUA"] if entry[1] == chip_number]
                if chip_data:  # Only update if there's new data
                    canvas.plot_iAQUA_chip_data(
                        chip_address=chip_number,
                        graph_data=sensor_data,
                        window_start=current_window_start,
                        window_end=window_end,
                    )
            elif key == "IMB":
                imb_data = sensor_data.get("IMB", [])
                if imb_data:  # Only update if there's new data
                    canvas.plot_IMB_data(
                        graph_data=sensor_data,
                        window_start=current_window_start,
                        window_end=window_end,
                    )
        except RuntimeError as e:
            print(f"Error updating graph for {key}: {e}")


###################################################################################################################
##########Threads
###################################################################################################################
# Start the MQTT client and CSV writer threads
if __name__ == "__main__":
    client = connect(broker_address, topic)

    # Start the MQTT loop in a separate thread
    mqtt_thread = threading.Thread(target=client.loop_start)
    mqtt_thread.start()

    # Start the CSV saving function in a separate thread
    csv_thread = threading.Thread(target=save_to_csv)
    csv_thread.start()

    # Start the plot windows
    plot_windows()
