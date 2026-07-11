# Running Penrose

This guide covers how to install dependencies and run each Penrose instance:

1. **GPU real-time interactive engine** (`Penrose`)
2. **Frame capture** and **PPM → video** utilities
3. **CPU physics benchmarking** (`benchmark_test`)

Penrose has two independent pipelines:

| Pipeline | Location | Executable / tool |
|---|---|---|
| GPU real-time | `realtime/` | `Penrose` |
| CPU scientific | `physics/` | `benchmark_test` |

---

## 1. Real-Time Interactive Engine

### Prerequisites

- C++20 compiler
  - **Windows**: Visual Studio (MSVC) or another CMake-supported toolchain
  - **Linux**: GCC or Clang
  - **macOS**: Clang via Xcode Command Line Tools
- CMake 3.22 or newer
- [vcpkg](https://github.com/microsoft/vcpkg) for package management
- OpenGL-capable GPU/drivers

Declared C++ dependencies (`vcpkg.json`): `glfw3`, `glad`, `glm`, `eigen3`.

GLAD is built from sources under `realtime/gpu/`. GLFW is fetched by CMake if needed.

### Install vcpkg

Clone and bootstrap vcpkg somewhere on your machine (not necessarily inside the Penrose tree):

**Windows (PowerShell):**

```powershell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

**Linux / macOS:**

```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
```

Remember the absolute path to the vcpkg root. You will pass it to CMake as:

```text
[PATH_TO_VCPKG]/scripts/buildsystems/vcpkg.cmake
```

If a local `vcpkg/` directory exists at the Penrose repository root, `CMakeLists.txt` may pick up its toolchain automatically.

### Build the visualizer

From the Penrose repository root:

**Windows:**

```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[PATH_TO_VCPKG]\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Debug
```

**Linux / macOS:**

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[PATH_TO_VCPKG]/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

### Run the visualizer

Run from the directory that contains the executable so the post-build-copied `shaders/` and `resources/` folders are found.

**Windows:**

```powershell
cd build\Debug
.\Penrose.exe
```

**Linux / macOS:**

```bash
./build/Penrose
```

Depending on generator/platform, the binary may live under a configuration subdirectory such as `build/Debug/Penrose`.

### Controls

| Input | Action |
|---|---|
| WASD | Move camera |
| Mouse | Look around when cursor is captured |
| Tab | Toggle cursor capture |
| Left / Right Shift | Sprint multiplier |
| P | Toggle frame capture |
| Escape | Exit or release/capture cursor (depending on input state) |

---

## 2. Frame Capture and PPM → Video

### Frame capture (inside the visualizer)

1. Start `Penrose` (see §1).
2. Press **P** to start capturing.
3. Fly / animate as desired.
4. Press **P** again to stop.

Console messages look like:

```text
Started capturing frames to: imagesequence/2026-05-20_14-30-45
Stopped capturing frames. Total frames saved: 300
```

#### Output layout

Frames are written as PPM files under a timestamped session folder:

```text
imagesequence/
└── YYYY-MM-DD_HH-MM-SS/
    ├── frame_000000.ppm
    ├── frame_000001.ppm
    └── ...
```

Notes:

- Capture runs every rendered frame while enabled (mostly disk I/O overhead).
- At 800×600, each PPM is ≈ 1.4 MB; 30 s at 30 fps ≈ 1.26 GB.
- Prefer a fast disk (SSD) for longer recordings.

More detail: [docs/frame_capture/FRAME_CAPTURE.md](frame_capture/FRAME_CAPTURE.md).

### Convert PPM sequences to video

#### Python helper (recommended)

Script: `realtime/visualization/ppm_to_video.py`

**Python dependencies** (also listed in `requirements.txt`):

```bash
pip install opencv-python numpy
```

**FFmpeg** must be installed and on `PATH`:

- **Windows**: `choco install ffmpeg`, or download from https://ffmpeg.org/download.html
- **macOS**: `brew install ffmpeg`
- **Linux**: e.g. `sudo apt install ffmpeg`

**Run** (from the repository root, or any directory):

```bash
python realtime/visualization/ppm_to_video.py
```

The script will:

1. Ask for the path to the `imagesequence` folder
2. List timestamped capture sessions
3. Let you pick a session
4. Ask for frame rate (default 30)
5. Write an MP4 under `videos/`

Example path prompts:

- `imagesequence`
- `/home/user/projects/penrose/imagesequence`
- `C:\Users\user\penrose\imagesequence`

Example output:

```text
videos/2026-05-20_14-30-45.mp4
```

More detail: [docs/frame_capture/PPM_TO_VIDEO_README.md](frame_capture/PPM_TO_VIDEO_README.md).

#### Manual FFmpeg alternative

```bash
ffmpeg -framerate 30 -pattern_type glob \
  -i 'imagesequence/2026-05-20_14-30-45/*.ppm' \
  -c:v libx264 -pix_fmt yuv420p output.mp4
```

---

## 3. Physics Benchmarking (CPU)

The CPU scientific pipeline lives under `physics/`. The CMake target `benchmark_test` runs freefall, orbital, and null-geodesic validation drivers from `physics/validation/`.

### Build

If you already configured the project in §1, you only need to build the benchmark target.

**Linux / macOS:**

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[PATH_TO_VCPKG]/scripts/buildsystems/vcpkg.cmake
cmake --build build --target benchmark_test
```

**Windows:**

```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[PATH_TO_VCPKG]\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Debug --target benchmark_test
```

`benchmark_test` links Eigen and does not require OpenGL/GLFW for execution.

### Run

**Linux / macOS:**

```bash
./build/benchmark_test
```

**Windows:**

```powershell
.\build\Debug\benchmark_test.exe
```

Null-geodesic cases can take several minutes.

### Outputs

CSV trajectories and diagnostics are written to:

```text
physics/results/data/
```

Examples: `freefall.csv`, `orbital.csv`, `null_b_*.csv`.

Analysis notebooks and plots live under `physics/analysis/`.

---

## Quick reference

| Goal | Command (after configure) |
|---|---|
| Build GPU visualizer | `cmake --build build` (Windows: add `--config Debug`) |
| Run GPU visualizer | `./build/Penrose` or `build\Debug\Penrose.exe` |
| Build CPU benchmarks | `cmake --build build --target benchmark_test` |
| Run CPU benchmarks | `./build/benchmark_test` or `build\Debug\benchmark_test.exe` |
| PPM → video | `python realtime/visualization/ppm_to_video.py` |

---

## Related docs

- [README.md](../README.md) — project overview and short build notes
- [docs/architecture/architecture_refactor.md](architecture/architecture_refactor.md) — architecture and pipeline structure
- [docs/frame_capture/FRAME_CAPTURE.md](frame_capture/FRAME_CAPTURE.md) — frame capture details
- [docs/frame_capture/PPM_TO_VIDEO_README.md](frame_capture/PPM_TO_VIDEO_README.md) — video conversion details
- [docs/reports/PROJECT_DOCUMENTATION.md](reports/PROJECT_DOCUMENTATION.md) — technical documentation
