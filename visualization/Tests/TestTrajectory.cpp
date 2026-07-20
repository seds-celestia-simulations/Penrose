#include "TestDeclarations.h"
#include "../Trajectory/TrajectoryAdapter.h"
#include <state/GeodesicState.h>

namespace viz::test {

bool test_trajectory_adapter_immutable_samples() {
    std::vector<State> states(2);
    states[0].X = Vector4d(0.0, 2.0, 1.5707963267948966, 0.0);
    states[1].X = Vector4d(0.1, 2.5, 1.5707963267948966, 0.5);

    Trajectory traj = adapt_states(states);
    if (traj.sample_count() != 2) {
        return false;
    }
    return traj.samples()[0].position.x > 0.0f && traj.samples()[1].position.x != traj.samples()[0].position.x;
}

} // namespace viz::test
