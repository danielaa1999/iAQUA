import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os

def extract_stable_regions(file_path):
    df = pd.read_csv(file_path, header=None, delimiter=",", usecols=[0,1,2,3])
    df.columns = ["Time", "Chip", "Sensor", "Value"]

    df["Sensor"] = df["Sensor"].astype(str).str.strip().str.upper()
    orp_df = df[df["Sensor"] == "ORP"].reset_index(drop=True)

    if orp_df.empty:
        print("⚠️ No ORP data found.")
        return

    stable_regions = []
    temp_values = []
    start_idx = None

    for i in range(len(orp_df)):
        current_value = orp_df.loc[i, "Value"]
        
        if current_value < 100 or current_value >= 1000:
            if len(temp_values) >= 10:
                stable_mean = np.mean(temp_values)
                stable_regions.append({"Start Index": start_idx, "End Index": i - 1, "Mean": stable_mean})
            temp_values = []
            start_idx = None
            continue

        if not temp_values:
            temp_values = [current_value]
            start_idx = i
        else:
            if abs(current_value - np.mean(temp_values)) <= 5:
                temp_values.append(current_value)
            else:
                if len(temp_values) >= 10:
                    stable_mean = np.mean(temp_values)
                    stable_regions.append({"Start Index": start_idx, "End Index": i - 1, "Mean": stable_mean})
                temp_values = [current_value]
                start_idx = i

    if len(temp_values) >= 10:
        stable_mean = np.mean(temp_values)
        stable_regions.append({"Start Index": start_idx, "End Index": len(orp_df) - 1, "Mean": stable_mean})

    # Merge adjacent stable regions within 50 units of each other
    merged_regions = []
    previous_region = None

    for region in stable_regions:
        if previous_region and abs(region["Mean"] - previous_region["Mean"]) <= 50:
            previous_region["End Index"] = region["End Index"]
            previous_region["Mean"] = (previous_region["Mean"] + region["Mean"]) / 2
        else:
            if previous_region:
                merged_regions.append(previous_region)
            previous_region = region

    if previous_region:
        merged_regions.append(previous_region)
    
    assigned_voltages = [0.22, 0.468]
    voltage_index = 0
    final_regions = []

    for region in merged_regions:
        start_idx = region["Start Index"]
        end_idx = region["End Index"]
        mean_value = region["Mean"]
        
        assigned_voltage = assigned_voltages[voltage_index % 2]
        voltage_index += 1
        
        final_regions.append({
            "Start Time": orp_df.loc[start_idx, "Time"],
            "End Time": orp_df.loc[end_idx, "Time"],
            "Mean ORP (Value)": mean_value,
            "Assigned Voltage(V)": assigned_voltage
        })
    
    stable_df = pd.DataFrame(final_regions)
    stable_df.to_csv("output_orp.csv", index=False)
    
    # Plot raw data
    plt.figure(figsize=(10, 6))
    plt.plot(orp_df["Time"], orp_df["Value"], label="ORP Data", color="blue")
    plt.xlabel("Time")
    plt.ylabel("Voltage (mV)")
    csv_filename = os.path.basename(file_path)
    plot_filename = csv_filename.replace(".csv", ".png")
    plt.title(csv_filename.replace(".csv", ""))
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.savefig(plot_filename)
    plt.close()
    
    print(f"📁 CSV and graph saved as {plot_filename}")

if __name__ == "__main__":
    extract_stable_regions("orp__electrochemical.csv")
