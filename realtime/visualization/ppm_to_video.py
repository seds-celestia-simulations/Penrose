import os
import sys
from pathlib import Path
import cv2
import numpy as np


def get_imagesequence_path():
    """Get the path to the imagesequence folder from user input."""
    print("\n" + "="*60)
    print("Image Sequence to Video Converter (PPM Format)")
    print("="*60)
    
    while True:
        print("\nEnter the path to the 'imagesequence' folder.")
        print("Examples:")
        print("  - imagesequence")
        print("  - /home/user/penrose/imagesequence")
        print("  - C:\\Users\\user\\penrose\\imagesequence")
        print("  - . (current directory, folder must be named 'imagesequence')")
        
        user_input = input("\nPath to imagesequence folder: ").strip()
        
        if not user_input:
            print("Path cannot be empty!")
            continue
        
        # If user just enters a folder name, assume it's in current directory
        if user_input == ".":
            path = "imagesequence"
        else:
            path = user_input
        
        if os.path.exists(path) and os.path.isdir(path):
            print(f"✓ Found imagesequence folder at: {os.path.abspath(path)}")
            return path
        else:
            print(f"✗ Error: Path not found or is not a directory: {path}")
            retry = input("Try again? (y/n) [default: y]: ").strip().lower()
            if retry == 'n':
                return None


def get_image_sequence_folders(imagesequence_path):
    """Get all timestamp folders in the imagesequence directory."""
    folders = []
    
    try:
        folders = [d for d in os.listdir(imagesequence_path) 
                   if os.path.isdir(os.path.join(imagesequence_path, d))]
    except PermissionError:
        print(f"Error: Permission denied accessing {imagesequence_path}")
        return []
    
    return sorted(folders, reverse=True)  # Most recent first


def display_folder_menu(folders, imagesequence_path):
    """Display menu to select a folder."""
    if not folders:
        print("No image sequence folders found!")
        return None
    
    print("\n" + "="*60)
    print("Available Image Sequences:")
    print("="*60)
    
    for i, folder in enumerate(folders, 1):
        num_images = len([f for f in os.listdir(os.path.join(imagesequence_path, folder)) 
                         if f.endswith('.ppm')])
        print(f"{i}. {folder} ({num_images} frames)")
    
    print("="*60)
    
    while True:
        try:
            choice = input(f"\nSelect a folder (1-{len(folders)}) or 'q' to quit: ").strip()
            
            if choice.lower() == 'q':
                return None
            
            choice_num = int(choice)
            if 1 <= choice_num <= len(folders):
                return folders[choice_num - 1]
            else:
                print(f"Invalid choice. Please enter a number between 1 and {len(folders)}.")
        except ValueError:
            print("Invalid input. Please enter a number or 'q'.")


def get_frame_rate():
    """Get frame rate from user."""
    print("\n" + "="*60)
    print("Frame Rate Selection:")
    print("="*60)
    print("Common frame rates: 24, 30, 60")
    
    while True:
        try:
            fps = float(input("\nEnter frame rate (fps) [default: 30]: ").strip() or "30")
            if fps > 0:
                return fps
            else:
                print("Frame rate must be positive!")
        except ValueError:
            print("Invalid input. Please enter a valid number.")


def convert_sequence_to_video(imagesequence_path, folder_name, fps):
    """Convert PPM image sequence to video."""
    sequence_path = os.path.join(imagesequence_path, folder_name)
    
    # Get all PPM files sorted numerically
    image_files = sorted([f for f in os.listdir(sequence_path) if f.endswith('.ppm')],
                        key=lambda x: int(x.split('_')[1].split('.')[0]))
    
    if not image_files:
        print(f"No PPM images found in {sequence_path}")
        return False
    
    # Create videos folder if it doesn't exist
    videos_dir = 'videos'
    if not os.path.exists(videos_dir):
        os.makedirs(videos_dir)
        print(f"Created '{videos_dir}' folder.")
    
    # Read first image to get dimensions
    first_image_path = os.path.join(sequence_path, image_files[0])
    first_image = cv2.imread(first_image_path)
    
    if first_image is None:
        print(f"Error reading first image: {first_image_path}")
        return False
    
    height, width = first_image.shape[:2]
    
    # Create video writer - use h264 codec for MP4
    video_path = os.path.join(videos_dir, f"{folder_name}.mp4")
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    out = cv2.VideoWriter(video_path, fourcc, fps, (width, height))
    
    if not out.isOpened():
        print(f"Error: Could not create video writer for {video_path}")
        print("Make sure ffmpeg is installed and accessible.")
        return False
    
    print(f"\nConverting {len(image_files)} frames to video...")
    print(f"Resolution: {width}x{height}")
    print(f"Frame rate: {fps} fps")
    print(f"Output: {video_path}")
    
    # Write frames to video
    for i, filename in enumerate(image_files):
        filepath = os.path.join(sequence_path, filename)
        frame = cv2.imread(filepath)
        
        if frame is None:
            print(f"Warning: Could not read {filename}")
            continue
        
        out.write(frame)
        
        # Progress indicator
        if (i + 1) % max(1, len(image_files) // 10) == 0:
            progress = ((i + 1) / len(image_files)) * 100
            print(f"Progress: {progress:.0f}%")
    
    out.release()
    
    print("\n" + "="*60)
    print(f"✓ Video successfully created: {video_path}")
    print(f"  Duration: {len(image_files) / fps:.2f} seconds")
    print("="*60)
    
    return True


def main():
    """Main function."""
    print("\n" + "="*60)
    print("PPM Image Sequence to Video Converter")
    print("="*60)
    
    # Get imagesequence folder path
    imagesequence_path = get_imagesequence_path()
    
    if imagesequence_path is None:
        print("Cancelled.")
        return
    
    # Get available folders
    folders = get_image_sequence_folders(imagesequence_path)
    
    if not folders:
        print("No image sequences available in this folder.")
        return
    
    # Display menu and get selection
    selected_folder = display_folder_menu(folders, imagesequence_path)
    
    if selected_folder is None:
        print("Cancelled.")
        return
    
    # Get frame rate
    fps = get_frame_rate()
    
    # Convert to video
    success = convert_sequence_to_video(imagesequence_path, selected_folder, fps)
    
    if success:
        input("\nPress Enter to exit...")


if __name__ == "__main__":
    main()
