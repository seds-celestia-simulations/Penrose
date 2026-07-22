"""Shared paths and physical constants for benchmark analysis."""

from pathlib import Path

from physics.analysis.output_paths import (
    BENCHMARK_DATA_ROOT,
    VALIDATION_FIGURES_ROOT,
    ensure_output_tree,
    latest_run_dir,
)

# Schwarzschild radius in geometric units (physics convention).
RS = 1.0

# Critical photon impact parameter: b_crit = (3*sqrt(3)/2) * r_s
B_CRIT = (3.0 * (3.0**0.5) / 2.0) * RS

# Photon sphere radius: 1.5 * r_s
PHOTON_SPHERE_R = 1.5 * RS

REPO_ROOT = Path(__file__).resolve().parents[2]
ensure_output_tree()


def benchmark_data_dir() -> Path:
    """Most recent benchmark CSV run, or the root awaiting data."""
    return latest_run_dir(BENCHMARK_DATA_ROOT) or BENCHMARK_DATA_ROOT


# Hardcoded initial conditions from main_benchmark.cpp (read-only reference).
FREEFALL_R0 = 10.0
FREEFALL_DT = 0.001

ORBITAL_R0 = 6.0
ORBITAL_VR = 0.0
ORBITAL_VPH = 0.06
ORBITAL_DT = 0.01
ORBITAL_MAX_STEPS = 100_000

NULL_R0 = 10.0
NULL_E = 1.0
NULL_DT_DEFAULT = 0.0005
NULL_MAX_STEPS = 200_000

# Representative null geodesic cases (main_benchmark.cpp).
NULL_EPS_ESCAPE = 1e-3
NULL_EPS_CAPTURE = 1e-3
NULL_EPS_NEAR_CRITICAL = 1e-5
