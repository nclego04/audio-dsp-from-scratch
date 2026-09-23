import sys
import csv
import glob
import matplotlib.pyplot as plt


def load_csv(path):
    hz, mag = [], []
    with open(path) as f:
        reader = csv.reader(f)
        next(reader)  # header
        for row in reader:
            hz.append(float(row[0]))
            mag.append(float(row[1]))
    return hz, mag


# no args -> plot every *.csv in the current folder;
# args given -> plot just those files
paths = sys.argv[1:] or sorted(glob.glob("*.csv"))
if not paths:
    print("no CSVs found (usage: plot_spectrum.py a.csv b.csv ...)")
    sys.exit(1)

cols = 2 if len(paths) > 1 else 1
rows = (len(paths) + cols - 1) // cols
fig, axes = plt.subplots(rows, cols, figsize=(6 * cols, 4 * rows), squeeze=False)

for i, path in enumerate(paths):
    ax = axes[i // cols][i % cols]
    hz, mag = load_csv(path)

    # drop bin 0 (DC): log(0) is undefined
    ax.plot(hz[1:], mag[1:])
    ax.set_xscale("log")

    ax.set_xlabel("Frequency (Hz)")
    ax.set_ylabel("Magnitude (dB)")
    ax.set_title(path)
    ax.grid(True, which="both", alpha=0.3)

# hide any unused grid cells (e.g. 5 plots in a 2x3 grid)
for j in range(len(paths), rows * cols):
    axes[j // cols][j % cols].axis("off")

plt.tight_layout()
plt.show()