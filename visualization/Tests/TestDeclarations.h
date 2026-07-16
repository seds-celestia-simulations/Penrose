#pragma once

namespace viz::test {

bool test_spherical_to_cartesian_on_axis();
bool test_spherical_to_cartesian_equatorial();
bool test_trajectory_adapter_immutable_samples();
bool test_csv_schema_detection_missing_file();
bool test_camera_view_projection_finite();
bool test_deterministic_render_checksum();
bool test_golden_ppm_roundtrip();

} // namespace viz::test
