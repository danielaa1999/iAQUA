import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os

# Define the order of regD values sent (convert to hex)
regD_combinations = [hex(x) for x in range(0x00, 0x40)]  # 0x00 to 0x3F in hex

# Load the CSV file while handling the trailing comma
input_file = "iaqua_offset_0604.csv"
df = pd.read_csv(input_file, header=None, usecols=[0, 1, 2, 3])

# Assign proper column names
df.columns = ["Timestamp", "Chip", "ISFET", "Value"]

# Filter out data before timestamp 100000 ms
df = df[df["Timestamp"] >= 100000].reset_index(drop=True)

# Create 5-second bins (5000ms each)
df["TimeBin"] = (df["Timestamp"] // 5000)  # Group timestamps into 5-second intervals

# Assign each time bin a regD value (cycling through available regD values)
unique_bins = sorted(df["TimeBin"].unique())
bin_to_regD = {bin_id: regD_combinations[i % len(regD_combinations)] for i, bin_id in enumerate(unique_bins)}
df["regD"] = df["TimeBin"].map(bin_to_regD)

# Pivot Table 1: Average value for each ISFET within each regD (5s period)
table_1 = df.groupby(["regD", "ISFET"])["Value"].mean().unstack().reset_index()
table_1.columns.name = None  # Remove column index name

# Table 2: Find the regD where each ISFET has a value closest to zero
df["AbsValue"] = df["Value"].abs()
closest_rows = df.loc[df.groupby("ISFET")["AbsValue"].idxmin(), ["Timestamp", "ISFET", "regD", "Value"]]

# Convert regD to hexadecimal format explicitly
closest_rows["regD"] = closest_rows["regD"].apply(lambda x: f"{x}")  # Ensure regD stays in hex

# Save the results to a CSV file
output_file = "processed_results.csv"
with open(output_file, "w") as f:
    table_1.to_csv(f, index=False, sep=';', float_format="%.4f")
    f.write("\n\n")
    closest_rows.to_csv(f, index=False, sep=';', float_format="%.4f")

# Plot all raw data using the original Timestamp (ms)
plt.figure(figsize=(10, 6))
for isfet in df["ISFET"].unique():
    subset = df[df["ISFET"] == isfet]
    plt.plot(subset["Timestamp"], subset["Value"], linestyle='-', label=f"{isfet}")  # No markers, just lines

# Customize the plot
plt.xlabel("Time (ms)")  # Use raw timestamp in milliseconds
plt.ylabel("EPS")
plt.title("regD offset test chip 0604")
plt.legend(title="ISFET")
plt.grid(True)

# Save the plot as a PNG file (same name as CSV but with .png extension)
output_png = os.path.splitext(input_file)[0] + ".png"
plt.savefig(output_png, dpi=300, bbox_inches="tight")  # High-quality image

# Show the plot
plt.show()
