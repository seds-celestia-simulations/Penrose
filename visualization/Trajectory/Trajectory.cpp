#include "Trajectory.h"

#include <algorithm>
#include <cmath>

namespace viz {

Trajectory::Trajectory(std::string name, std::vector<TrajectorySample> samples, TrajectoryStyle style,
                       bool inferred_equatorial)
    : name_(std::move(name)), samples_(std::move(samples)), style_(style),
      inferred_equatorial_(inferred_equatorial) {}

void decimate_trajectory_samples(std::vector<TrajectorySample>& samples, std::size_t max_samples) {
    if (samples.size() <= max_samples || max_samples < 2) {
        return;
    }

    std::vector<TrajectorySample> reduced;
    reduced.reserve(max_samples);
    reduced.push_back(samples.front());

    const double step =
        static_cast<double>(samples.size() - 1) / static_cast<double>(max_samples - 1);
    for (std::size_t i = 1; i + 1 < max_samples; ++i) {
        const std::size_t idx = static_cast<std::size_t>(std::llround(step * static_cast<double>(i)));
        reduced.push_back(samples[std::min(idx, samples.size() - 1)]);
    }

    reduced.push_back(samples.back());
    samples = std::move(reduced);
}

double Trajectory::min_parameter() const {
    if (samples_.empty()) {
        return 0.0;
    }
    return samples_.front().parameter;
}

double Trajectory::max_parameter() const {
    if (samples_.empty()) {
        return 0.0;
    }
    return samples_.back().parameter;
}

std::size_t Trajectory::index_at_parameter(double value) const {
    if (samples_.empty()) {
        return 0;
    }
    auto it = std::upper_bound(samples_.begin(), samples_.end(), value,
                               [](double v, const TrajectorySample& s) { return v < s.parameter; });
    if (it == samples_.begin()) {
        return 0;
    }
    return static_cast<std::size_t>(std::distance(samples_.begin(), it - 1));
}

} // namespace viz
