#include "../Camera/Camera.h"
#include "../IO/CsvTrajectoryLoader.h"
#include "../IO/OutputPaths.h"
#include "../IO/PpmWriter.h"
#include "../Presentation/PresentationDefaults.h"
#include "../Presentation/PresentationPipeline.h"
#include "../Scene/SceneBuilder.h"

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct ExportOptions {
    int width = 1280;
    int height = 720;
    std::string output;
    std::string output_dir;
    int frame_count = 1;
    float camera_distance = -1.0f;
    float camera_yaw = -1.0f;
    float camera_pitch = -1.0f;
    double playback_time = -1.0;
    bool classic = false;
    std::vector<std::string> csv_paths;
};

void print_usage() {
    std::cout << "Penrose headless visualization export\n\n"
              << "Usage: visualization_export [options]\n\n"
              << "Options:\n"
              << "  --output PATH         Output PPM path (default: outputs/rendered_frames/<timestamp>/frame.ppm)\n"
              << "  --output-dir PATH     Directory for frame sequence (default: new timestamped run)\n"
              << "  --frames N            Number of frames to export (default: 1)\n"
              << "  --width W             Image width (default: 1280)\n"
              << "  --height H            Image height (default: 720)\n"
              << "  --csv PATH            Benchmark CSV trajectory (repeatable)\n"
              << "  --camera-distance D   Orbit camera distance\n"
              << "  --camera-yaw Y        Camera yaw radians\n"
              << "  --camera-pitch P      Camera pitch radians\n"
              << "  --time T              Playback time; default uses final frame for stills\n"
              << "  --classic             Disable cinematic presentation post-processing\n"
              << "  --help                Show this help\n";
}

bool parse_args(int argc, char** argv, ExportOptions& opts) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage();
            return false;
        }
        if (arg == "--output" && i + 1 < argc) {
            opts.output = argv[++i];
        } else if (arg == "--output-dir" && i + 1 < argc) {
            opts.output_dir = argv[++i];
        } else if (arg == "--frames" && i + 1 < argc) {
            opts.frame_count = std::stoi(argv[++i]);
        } else if (arg == "--width" && i + 1 < argc) {
            opts.width = std::stoi(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            opts.height = std::stoi(argv[++i]);
        } else if (arg == "--csv" && i + 1 < argc) {
            opts.csv_paths.push_back(argv[++i]);
        } else if (arg == "--camera-distance" && i + 1 < argc) {
            opts.camera_distance = std::stof(argv[++i]);
        } else if (arg == "--camera-yaw" && i + 1 < argc) {
            opts.camera_yaw = std::stof(argv[++i]);
        } else if (arg == "--camera-pitch" && i + 1 < argc) {
            opts.camera_pitch = std::stof(argv[++i]);
        } else if (arg == "--time" && i + 1 < argc) {
            opts.playback_time = std::stod(argv[++i]);
        } else if (arg == "--classic") {
            opts.classic = true;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            print_usage();
            return false;
        }
    }
    return true;
}

std::string frame_path(const ExportOptions& opts, int frame) {
    if (opts.output_dir.empty()) {
        return opts.output;
    }
    char name[64];
    std::snprintf(name, sizeof(name), "%s/frame_%06d.ppm", opts.output_dir.c_str(), frame);
    return name;
}

} // namespace

int main(int argc, char** argv) {
    ExportOptions opts;
    if (!parse_args(argc, argv, opts)) {
        return 0;
    }

    const auto repo_root = viz::detect_repo_root();
    if (opts.output_dir.empty() && opts.output.empty()) {
        if (opts.frame_count > 1) {
            opts.output_dir = viz::default_render_sequence_dir(repo_root).string();
        } else {
            opts.output = viz::default_render_still_path(repo_root).string();
        }
    } else if (opts.output_dir.empty() && opts.frame_count > 1) {
        opts.output_dir = viz::default_render_sequence_dir(repo_root).string();
    }

    viz::Scene scene = opts.classic ? viz::SceneBuilder::default_schwarzschild_scene()
                                    : viz::Scene(viz::cinematic_scene_settings());
    for (const std::string& csv : opts.csv_paths) {
        if (auto loaded = viz::load_trajectory_csv(csv)) {
            scene.add_trajectory(std::move(loaded->trajectory));
            std::cout << loaded->message << " (" << csv << ")\n";
        } else {
            std::cerr << "Failed to load CSV: " << csv << "\n";
            return 1;
        }
    }

    viz::Camera camera;
    if (opts.classic) {
        camera.set_distance(12.0f);
        camera.set_angles(0.6f, 0.35f);
    } else {
        viz::apply_cinematic_camera(camera);
    }
    if (opts.camera_distance >= 0.0f) {
        camera.set_distance(opts.camera_distance);
    }
    if (opts.camera_yaw >= 0.0f && opts.camera_pitch >= 0.0f) {
        camera.set_angles(opts.camera_yaw, opts.camera_pitch);
    }

    viz::Framebuffer framebuffer(opts.width, opts.height);
    viz::PresentationPipeline pipeline;
    viz::PresentationProfile profile = viz::PresentationProfile::cinematic_default();
    profile.enabled = !opts.classic;

    const int frames = std::max(1, opts.frame_count);
    for (int frame = 0; frame < frames; ++frame) {
        if (opts.playback_time >= 0.0) {
            scene.playback().time = opts.playback_time;
        } else if (frames == 1) {
            scene.playback().time = scene.playback().duration;
        } else {
            scene.playback().time = scene.playback().duration * (static_cast<double>(frame) /
                                                                   static_cast<double>(frames - 1));
        }

        pipeline.render(scene, camera, framebuffer, {}, profile);
        const std::string path = frame_path(opts, frame);
        if (!viz::write_ppm(framebuffer, path)) {
            std::cerr << "Failed to write " << path << "\n";
            return 1;
        }
        std::cout << "Wrote " << path << " (t=" << scene.playback().time << ")\n";
    }

    return 0;
}
