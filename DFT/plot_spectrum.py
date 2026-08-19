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


def find_peaks(mag, count=5, guard=5):
    work = list(mag)
    peaks = []
    for _ in range(count):
        k = max(range(len(work)), key=lambda i: work[i])
        if work[k] <= 0:
            break
        peaks.append(k)
        lo, hi = max(0, k - guard), min(len(work) - 1, k + guard)
        for j in range(lo, hi + 1):
            work[j] = 0
    return peaks


# no args -> plot every *_spectrum.csv in the current folder;
# args given -> plot just those files
paths = sys.argv[1:] or sorted(glob.glob("*_spectrum.csv"))
if not paths:
    print("no spectrum CSVs found (usage: plot_spectrum.py a.csv b.csv ...)")
    sys.exit(1)

cols = 2 if len(paths) > 1 else 1
rows = (len(paths) + cols - 1) // cols
fig, axes = plt.subplots(rows, cols, figsize=(6 * cols, 4 * rows), squeeze=False)

for i, path in enumerate(paths):
    ax = axes[i // cols][i % cols]
    hz, mag = load_csv(path)
    peaks = find_peaks(mag, count=5)

    ax.plot(hz, mag)
    ax.plot([hz[k] for k in peaks], [mag[k] for k in peaks], "ro")
    for k in peaks:
        ax.annotate(f"{hz[k]:.0f} Hz", xy=(hz[k], mag[k]), xytext=(0, 8),
                    textcoords="offset points", ha="center", fontsize=8)

    ax.set_xlabel("Frequency (Hz)")
    ax.set_ylabel("Magnitude")
    ax.set_title(path)

# hide any unused grid cells (e.g. 5 plots in a 2x3 grid)
for j in range(len(paths), rows * cols):
    axes[j // cols][j % cols].axis("off")

plt.tight_layout()
plt.show()
