#pragma once

namespace Spacetime {

// Metric-specific parameters for Schwarzschild. Not part of SimulationConfig.
struct SchwarzschildParameters {
    double mass = 1.0; // Schwarzschild radius rs in code units where G=c=1
};

// Future examples (add alongside new Metric implementations — do not expand SimulationConfig):
// struct KerrParameters { double mass = 1.0; double spin = 0.0; };
// struct FlrwParameters { double OmegaM = 0.3; double OmegaLambda = 0.7; ... };
// struct SglParameters { double observer_distance = ...; double lens_distance = ...; ... };

} // namespace Spacetime
