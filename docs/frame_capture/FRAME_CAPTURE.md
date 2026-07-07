# Frame Capture Feature

## Overview
The frame capture feature allows you to record sequences of rendered frames to disk. This is useful for creating animations, debugging visualizations, or generating video sequences.

## How to Use

### Starting and Stopping Capture
- **Press the "P" key** to toggle frame capture mode:
  - **First press**: Starts capturing frames
  - **Second press**: Stops capturing frames

### Output Format
- **Frame Format**: PPM (Portable Pixmap) format
- **Naming Convention**: `frame_XXXXXX.ppm` (6-digit zero-padded numbering)
- **Output Location**: Frames are saved in a timestamped directory structure:
  ```
  imagesequence/
  ├── 2026-05-20_14-30-45/
  │   ├── frame_000000.ppm
  │   ├── frame_000001.ppm
  │   ├── frame_000002.ppm
  │   └── ...
  └── 2026-05-20_15-22-10/
      ├── frame_000000.ppm
      └── ...
  ```

### Timestamps
The directory name uses the format: `YYYY-MM-DD_HH-MM-SS`

Each capture session creates a new timestamped directory, so multiple captures won't interfere with each other.

## Technical Details

### Implementation Files
1. **FrameCapture.h** - Main capture class that manages:
   - Toggle state for capture mode
   - Timestamped directory creation
   - Frame numbering and file path generation

2. **Renderer.cpp (captureFrame method)** - Handles:
   - Reading pixels from OpenGL framebuffer using `glReadPixels()`
   - Writing PPM format files
   - Vertical flip correction (OpenGL reads from bottom-left)

3. **Window.cpp** - Input handling:
   - Detects 'P' key press
   - Toggles capture state via `toggleCapture()`
   - Uses static key state tracking to prevent multiple toggles per frame

4. **main.cpp** - Integration:
   - Creates FrameCapture instance
   - Passes it to `processInput()` 
   - Calls `captureFrame()` after rendering each frame when capturing is enabled

## Console Output
When you press 'P', you'll see console messages:
- **Start**: `Started capturing frames to: imagesequence/2026-05-20_14-30-45`
- **Stop**: `Stopped capturing frames. Total frames saved: 300`

## Converting PPM to Video
PPM files can be converted to video formats using tools like FFmpeg:

```bash
ffmpeg -framerate 30 -pattern_type glob -i 'imagesequence/2026-05-20_14-30-45/*.ppm' -c:v libx264 -pix_fmt yuv420p output.mp4
```

## Performance Notes
- Frame capture happens every frame while recording
- Capturing adds minimal overhead (mostly disk I/O)
- Frame capture runs synchronously, so very fast disk writes are recommended
- Consider using an SSD for better performance

## File Size Estimation
For 800x600 resolution:
- Each PPM frame ≈ 1.4 MB
- 30 seconds of footage ≈ 1.26 GB (at 30 fps)
- 60 seconds of footage ≈ 2.52 GB (at 30 fps)

## Future Enhancements
Possible improvements:
1. Support for PNG format (with compression)
2. Adjustable frame rate independent of render frame rate
3. Recording pause/resume functionality
4. Video encoding directly to MP4/WebM
5. Selectable output directory
