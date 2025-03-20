import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import linregress

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
        
        if current_value <= -16500:
            if len(temp_values) >= 3:
                stable_mean = np.mean(temp_values)
                stable_regions.append({"Start Index": start_idx, "End Index": i - 1, "Mean": stable_mean})
            temp_values = []
            start_idx = None
            continue

        if not temp_values:
            temp_values = [current_value]
            start_idx = i
        else:
            if abs(current_value - np.mean(temp_values)) <= 100:
                temp_values.append(current_value)
            else:
                if len(temp_values) >= 3:
                    stable_mean = np.mean(temp_values)
                    stable_regions.append({"Start Index": start_idx, "End Index": i - 1, "Mean": stable_mean})
                temp_values = [current_value]
                start_idx = i

    if len(temp_values) >= 3:
        stable_mean = np.mean(temp_values)
        stable_regions.append({"Start Index": start_idx, "End Index": len(orp_df) - 1, "Mean": stable_mean})

    merged_regions = []
    for region in stable_regions:
        start_idx = region["Start Index"]
        end_idx = region["End Index"]
        mean_value = region["Mean"]
        
        if mean_value > 10000:
            continue  # Ignore values above the range
        
        merged_regions.append({
            "Start Time": orp_df.loc[start_idx, "Time"],
            "End Time": orp_df.loc[end_idx, "Time"],
            "Mean ORP (Value)": mean_value
        })
    
    stable_df = pd.DataFrame(merged_regions)
    voltage_values = [0, 0.2, 0.3, 0.4, 0.5]
    assigned_voltages = []
    assigned_means = []
    valid_rows = stable_df.dropna().index

    for row_idx in valid_rows:
        mean_value = stable_df.at[row_idx, "Mean ORP (Value)"]

        if mean_value < 1000 and 0.0 not in assigned_voltages:
            stable_df.at[row_idx, "Voltage"] = 0.0
            assigned_voltages.append(0.0)
            assigned_means.append(mean_value)
        elif mean_value >= 1000:
            for voltage in voltage_values[len(assigned_voltages):]:
                if all(abs(mean_value - prev_mean) >= 1000 for prev_mean in assigned_means):
                    stable_df.at[row_idx, "Voltage"] = voltage
                    assigned_voltages.append(voltage)
                    assigned_means.append(mean_value)
                    break

    stable_df = stable_df.dropna(subset=["Voltage"])  # Ensure only assigned voltage rows are used
    voltage_adc_mapping = stable_df.groupby("Voltage", as_index=False).mean()
    
    x = voltage_adc_mapping["Voltage"]
    y = voltage_adc_mapping["Mean ORP (Value)"]
    slope, intercept, _, _, _ = linregress(x, y)
    regression_formula = f"y = {slope:.4f}x + {intercept:.4f}"

    plt.figure(figsize=(10, 6))
    plt.scatter(x, y, color="blue", label="Measured ADC Values")
    plt.plot(x, slope * x + intercept, color="red", linestyle="--", label="Linear Regression")
    plt.xlabel("Voltage (V)")
    plt.ylabel("Mean ORP (Value)")
    plt.title("Linear Regression of Voltage vs. ADC Output")
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.savefig("linear_regression.png")
    plt.close()

    stable_df.to_csv("output_orp.csv", index=False)
    with open("output_orp.csv", "a") as f:
        f.write("\n\n\n")
        f.write(f"Linear Regression Formula: {regression_formula}\n")
    
    print(f"📁 CSV saved with regression formula")

if __name__ == "__main__":
    extract_stable_regions("IMB_ORP_PGA_2.csv")