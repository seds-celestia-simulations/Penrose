#include "TestDeclarations.h"
#include "../IO/CsvTrajectoryLoader.h"

namespace viz::test {

bool test_csv_schema_detection_missing_file() {
    return !load_trajectory_csv("/nonexistent/path.csv").has_value();
}

} // namespace viz::test
