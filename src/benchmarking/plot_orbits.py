import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os

plt.style.use('dark_background')

rs = 1.0
r_photon = 1.5 * rs


def plot_trajectory(ax, filename, color, label):
    script_dir = os.path.dirname(os.path.abspath(__file__))
    path = os.path.join(script_dir, filename)

    try:
        df = pd.read_csv(path)

        phi_vals = df['phi'] if 'phi' in df.columns else np.zeros(len(df))

        x = df['r'] * np.cos(phi_vals)
        y = df['r'] * np.sin(phi_vals)

        # trajectory
        ax.plot(x, y, color=color, label=label, linewidth=2, alpha=0.85)

        # start point
        ax.scatter(x.iloc[0], y.iloc[0], color=color, s=40, edgecolors='white', zorder=12)

        # end point
        ax.scatter(x.iloc[-1], y.iloc[-1], color='red', marker='x', s=50, zorder=12)

        print(f"[OK] {filename}")
        return True

    except FileNotFoundError:
        print(f"[!] Missing: {filename}")
        return False

    except Exception as e:
        print(f"[!] Error in {filename}: {e}")
        return False


# ---- Figure ----
fig, ax = plt.subplots(figsize=(10, 10))

# Black hole
ax.add_patch(plt.Circle((0, 0), rs, color='black', zorder=10))
ax.add_patch(plt.Circle((0, 0), rs, color='white', fill=False, linewidth=2, zorder=11))

# Photon sphere
ax.add_patch(plt.Circle((0, 0), r_photon, color='orange',
                        fill=False, linestyle='--', alpha=0.5,
                        label='Photon Sphere'))

# ---- Load trajectories ----
has_plots = []
has_plots.append(plot_trajectory(ax, 'data/orbital.csv', 'dodgerblue', 'Massive Orbit'))
has_plots.append(plot_trajectory(ax, 'data/null_geodesic.csv', 'yellow', 'Photon Path'))
has_plots.append(plot_trajectory(ax, 'data/freefall.csv', 'lime', 'Freefall'))

# ---- Formatting ----
ax.set_aspect('equal')
ax.set_xlim(-12, 12)
ax.set_ylim(-12, 12)

ax.set_xlabel('X (Geometric Units)')
ax.set_ylabel('Y (Geometric Units)')
ax.set_title('Schwarzschild Raytracer Benchmark', fontsize=14)

ax.grid(True, linestyle=':', alpha=0.3)

if any(has_plots):
    ax.legend()

plt.tight_layout()
plt.show()