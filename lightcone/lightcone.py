import numpy as np
import matplotlib.pyplot as plt


# --------------------------------------------------
# Grid
# --------------------------------------------------

class Grid:
    def __init__(self, t, r):
        self.t = t
        self.r = r


# --------------------------------------------------
# Schwarzschild Metric
# --------------------------------------------------

class SchwarzschildMetric:
    def __init__(self, rs):
        self.rs = rs

    def g(self, r):
        return np.array([
            [-(1 - self.rs / r), 0],
            [0, 1 / (1 - self.rs / r)]
        ])


# --------------------------------------------------
# Light Cones
# --------------------------------------------------

class Lightcone:
    def __init__(self, metric):
        self.metric = metric

    def compute_lightcone(self, r):
        g = self.metric.g(r)

        g_tt = g[0, 0]
        g_rr = g[1, 1]

        dr_dt = np.sqrt(-g_tt / g_rr)
        return dr_dt, -dr_dt

    def draw_cone(self, ax, r, t, slopes, size=0.15):

        for dr_dt in slopes:

            dt = 1.0
            dr = dr_dt

            # Normalize so every cone has the same length
            norm = np.sqrt(dr**2 + dt**2)

            dr *= size / norm
            dt *= size / norm

            ax.plot(
                [r - dr, r + dr],
                [t - dt, t + dt],
                color="royalblue",
                lw=1
            )


# --------------------------------------------------
# Photon
# --------------------------------------------------

class Photon:

    def __init__(self, r0, outgoing=True):

        self.r = r0
        self.t = 0.0

        self.outgoing = outgoing

        self.history_r = [r0]
        self.history_t = [0.0]

    def step(self, dt, metric):

        sign = 1 if self.outgoing else -1

        # dr/dt
        dr_dt = sign * (1 - metric.rs / self.r)

        self.r += dr_dt * dt
        self.t += dt

        self.history_r.append(self.r)
        self.history_t.append(self.t)


# --------------------------------------------------
# Plotter
# --------------------------------------------------

class LightconePlotter:

    def __init__(self, lightcone, grid):
        self.lightcone = lightcone
        self.grid = grid

    def draw_background(self, ax):

        for t in self.grid.t:
            for r in self.grid.r:

                if r <= self.lightcone.metric.rs:
                    continue

                slopes = self.lightcone.compute_lightcone(r)

                self.lightcone.draw_cone(ax, r, t, slopes)

        ax.axvline(
            self.lightcone.metric.rs,
            color="red",
            linestyle="--",
            label="Event Horizon"
        )

    def draw_photons(self, ax, photons):

        colors = ["orange", "limegreen", "magenta", "black"]

        for i, photon in enumerate(photons):

            ax.plot(
                photon.history_r,
                photon.history_t,
                color=colors[i % len(colors)],
                lw=2
            )

            ax.scatter(
                photon.r,
                photon.t,
                color=colors[i % len(colors)],
                s=25
            )

    def show(self, photons):

        fig, ax = plt.subplots(figsize=(8, 8))

        self.draw_background(ax)
        self.draw_photons(ax, photons)

        ax.set_xlim(0.8, 8)
        ax.set_ylim(0, 10)

        ax.set_xlabel("Radius $r$")
        ax.set_ylabel("Time $t$")
        ax.set_title("Schwarzschild Light Cones + Photon Worldlines")

        ax.legend()

        plt.show()


# --------------------------------------------------
# Main
# --------------------------------------------------

if __name__ == "__main__":

    rs = 1.0

    grid = Grid(
        t=np.linspace(0, 10, 15),
        r=np.linspace(1.2, 8, 20)
    )

    metric = SchwarzschildMetric(rs)

    lightcone = Lightcone(metric)

    plotter = LightconePlotter(lightcone, grid)

    photons = [

        Photon(6.0, outgoing=True),
        Photon(3.0, outgoing=True),

        Photon(5.0, outgoing=False),
        Photon(2.0, outgoing=False),

    ]

    dt = 0.00001

    steps = 500

    for _ in range(steps):

        for photon in photons:

            if photon.r <= rs:
                continue
            while rs < photon.r < 8 and photon.t < 10:
                photon.step(dt, metric)


    plotter.show(photons)