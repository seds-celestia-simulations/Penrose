#pragma once
#include "../state/State.h"

// Radial free-fall benchmark.
// Drops a particle from rest at r0 with E=1 (released from infinity).
// Compares numerical horizon-crossing time to the exact analytical result.
// Writes trajectory to freefall.csv.
void benchmark_freefall(double r0, double dt);
