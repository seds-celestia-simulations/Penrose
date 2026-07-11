#pragma once

#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <iostream>

class FrameCapture {
private:
    bool isCapturing;
    std::string captureDirectory;
    int frameCount;

    std::string generateTimestampedDirectory() {
        // Get current time
        auto now = std::time(nullptr);
        auto tm = std::localtime(&now);
        
        // Create timestamp string: YYYY-MM-DD_HH-MM-SS
        std::ostringstream oss;
        oss << std::put_time(tm, "%Y-%m-%d_%H-%M-%S");
        
        std::string baseDir = "imagesequence";
        std::string timestampedDir = baseDir + "/" + oss.str();
        
        // Create directories if they don't exist
        try {
            std::filesystem::create_directories(timestampedDir);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error creating directory: " << e.what() << std::endl;
            return "";
        }
        
        return timestampedDir;
    }

public:
    FrameCapture() : isCapturing(false), frameCount(0) {}

    void toggleCapture() {
        if (!isCapturing) {
            // Start capturing
            captureDirectory = generateTimestampedDirectory();
            if (!captureDirectory.empty()) {
                isCapturing = true;
                frameCount = 0;
                std::cout << "Started capturing frames to: " << captureDirectory << std::endl;
            }
        } else {
            // Stop capturing
            std::cout << "Stopped capturing frames. Total frames saved: " << frameCount << std::endl;
            isCapturing = false;
            frameCount = 0;
        }
    }

    bool getIsCapturing() const {
        return isCapturing;
    }

    std::string getNextFilePath() {
        if (!isCapturing) return "";
        
        std::ostringstream oss;
        oss << captureDirectory << "/frame_"
            << std::setfill('0') << std::setw(6) << frameCount << ".ppm";
        frameCount++;
        
        return oss.str();
    }

    int getFrameCount() const {
        return frameCount;
    }
};
