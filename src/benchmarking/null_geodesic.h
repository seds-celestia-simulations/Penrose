#pragma once
#include "../core/State.h"

// Null geodesic benchmark (Light Rays).
// Supply r0, vr, vph and dt freely.
// vt is computed automatically from the null condition g_uv U^u U^v = 0.
// Excellent for testing the unstable photon sphere at r = 1.5 * rs.
//
// Writes full trajectory and metric norm to null_geodesic.csv.
void benchmark_null_geodesic(double r0, double vr, double vph, double dt, int max_steps);