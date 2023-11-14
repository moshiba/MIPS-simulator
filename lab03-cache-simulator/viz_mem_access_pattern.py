"""Visualize data access pattern.
Colors read in blue and write in red.
Zoom with mouse.
"""

import matplotlib.pyplot as plt

with open("trace.txt", "rt") as f:
    lines = [line.strip() for line in f]

x, y, colors = [], [], []
for line, idx in zip(lines, range(len(lines))):
    mode, addr = line.split()
    addr = int(addr, 16)

    x.append(idx)
    y.append(addr)

    if mode == "R":
        colors.append(-1)
    elif mode == "W":
        colors.append(1)
    else:
        colors.append(0)

plt.scatter(x, y, c=colors, alpha=0.5, cmap="coolwarm")
plt.xlabel = "Time"
plt.ylabel = "Address"
plt.show()
