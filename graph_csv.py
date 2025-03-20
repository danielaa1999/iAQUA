import pandas as pd
import os
import matplotlib.pyplot as plt

# Define folder path
folder_path = r'c:\Users\users\Desktop\Daniela\results\pruebas_119\bad'

# Get all CSV files in the folder, excluding "_stable_averages.csv" files
csv_files = [f for f in os.listdir(folder_path) if f.lower().endswith('.csv') and "_stable_averages" not in f]

for file_name in csv_files:
    file_path = os.path.join(folder_path, file_name)
    print(f"\nProcessing: {file_path}")

    # Read CSV and drop empty columns
    data = pd.read_csv(file_path, header=None).dropna(axis=1, how='all')

    # Ensure it has exactly 4 columns
    if data.shape[1] != 4:
        print(f"Skipping {file_name}: Unexpected column count ({data.shape[1]})")
        continue

    # Assign correct column names
    data.columns = ['Time', 'Chip', 'ISFET', 'Value']

    # Convert time to seconds
    data['Time'] = data['Time'] / 1000.0

    # Function to plot data
    def plot_data(df, output_name, title):
        if df.empty:
            print(f"Skipping plot for '{output_name}': No data available.")
            return

        plt.figure(figsize=(12, 6))
        for isfet in df['ISFET'].unique():
            subset = df[df['ISFET'] == isfet]
            plt.plot(subset['Time'], subset['Value'], linewidth=1.5, label=isfet, alpha=0.7)

        plt.xlabel('Time (seconds)')
        plt.ylabel('Value')
        plt.legend()
        plt.title(title)
        plt.grid(True, linestyle='--', linewidth=0.5, alpha=0.7)

        output_path = os.path.join(folder_path, output_name)
        plt.savefig(output_path)
        plt.close()
        print(f"Graph saved as '{output_path}'")

    # ---------------------- PLOT RAW DATA ----------------------
    raw_output_name = f"{os.path.splitext(file_name)[0]}.png"

    # Ensure there's data to plot
    if data.empty:
        print(f"Skipping {file_name}: CSV is empty.")
    else:
        plot_data(data, raw_output_name, os.path.splitext(file_name)[0])
