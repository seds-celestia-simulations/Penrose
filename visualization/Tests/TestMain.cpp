#include "TestHarness.h"
#include "TestDeclarations.h"

int main() {
    using namespace viz::test;

    const std::vector<TestCase> tests = {
        {"spherical_to_cartesian_on_axis", test_spherical_to_cartesian_on_axis},
        {"spherical_to_cartesian_equatorial", test_spherical_to_cartesian_equatorial},
        {"trajectory_adapter_immutable_samples", test_trajectory_adapter_immutable_samples},
        {"csv_schema_detection_missing_file", test_csv_schema_detection_missing_file},
        {"camera_view_projection_finite", test_camera_view_projection_finite},
        {"deterministic_render_checksum", test_deterministic_render_checksum},
        {"golden_ppm_roundtrip", test_golden_ppm_roundtrip},
    };

    return run_all(tests);
}
