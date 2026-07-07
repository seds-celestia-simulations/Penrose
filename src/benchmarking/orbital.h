#pragma once
#include "../physics_engine/physics.h"

// General geodesic benchmark.
// Supply any r0, vr, vph and dt freely.
// vt is computed automatically from the norm condition g_uv U^u U^v = -1
// so the particle is always on a valid massive geodesic.
//
// Stops when:
//   - r <= rs        (horizon crossed)
//   - r > 1000       (escaped to infinity)
//   - max_steps hit  (time limit)
//
// Writes full trajectory to geodesic.csv.
void benchmark_orbital(double r0, double vr, double vph, double dt, int max_steps);
