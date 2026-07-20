#pragma once

namespace Constants {

    constexpr double G = 6.67430e-11;
    constexpr double c = 299792458.0;

    constexpr double solar_mass = 1.98847e30;

    inline double schwarzschild_radius(double mass) {
        return 2.0 * G * mass / (c * c);
    }

}
