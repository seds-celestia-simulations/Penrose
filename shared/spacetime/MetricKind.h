#pragma once

namespace Spacetime {

// Backend-independent metric identity vocabulary.
enum class MetricKind {
    Schwarzschild,
    // Future: Kerr, ReissnerNordstrom, FLRW, SolarGravitationalLens, ...
};

// Coordinate chart identity for state component ordering.
enum class CoordinateChartKind {
    SchwarzschildSpherical, // X = (t, r, theta, phi)
    // Future: KerrBoyerLindquist, Cartesian, ...
};

} // namespace Spacetime
