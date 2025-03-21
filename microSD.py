import struct
import os

def twos_complement(value, bit_width):
    """Convert a two's complement binary number to a signed integer."""
    if value & (1 << (bit_width - 1)):
        value -= 1 << bit_width
    return value

def get_sensor_type(board_type, sensor_type):
    """Return the correct sensor type string based on board type."""
    if board_type == 0:  # IMB
        if 0 <= sensor_type <= 6:
            return f"ISFET{sensor_type + 1}"
        elif sensor_type == 7:
            return "AMP"
        elif sensor_type == 8:
            return "ORP"
    elif board_type == 1:  # iAQUA
        if 1 <= sensor_type <= 9:
            return f"ISFET{sensor_type}"
        elif sensor_type == 10:
            return "AMP"
        elif sensor_type == 11:
            return "ORP"
    return "Unknown"

def interpret_binary_data(input_file):
    output_file = os.path.splitext(input_file)[0] + ".csv"

    with open(input_file, "r") as infile, open(output_file, "w") as outfile:
        outfile.write("Timestamp,Chip Address,Board Type,Sensor Type,Sensor Data,Checksum\n")

        for line in infile:
            binary_str = line.strip().replace(" ", "")

            if len(binary_str) != 72:
                print(f"Skipping invalid line (length {len(binary_str)}): {binary_str}")
                continue

            data_bits = binary_str[:64]
            checksum_bits = binary_str[64:]

            timestamp = int(data_bits[:32], 2)
            chip_address = int(data_bits[32:40], 2)
            board_type = int(data_bits[40], 2)
            sensor_type = int(data_bits[41:48], 2)
            sensor_data = twos_complement(int(data_bits[48:], 2), 16)
            checksum = int(checksum_bits, 2)

            board_name = "IMB" if board_type == 0 else "iAQUA"
            sensor_name = get_sensor_type(board_type, sensor_type)

            outfile.write(f"{timestamp},{chip_address},{board_name},{sensor_name},{sensor_data},{checksum}\n")

    print(f"Processed data saved to {output_file}")

# Usage example
interpret_binary_data("iaqua_0604_appllied_offset.txt")
