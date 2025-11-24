#!/bin/bash
# Script to create symlinks for OpenCV 4.13 libraries
# This fixes the missing libopencv_ml.so.413 and related libraries

LIB_DIR="/usr/lib/aarch64-linux-gnu"
REQUIRED_LIBS=(
    "libopencv_core.so.413"
    "libopencv_imgproc.so.413"
    "libopencv_imgcodecs.so.413"
    "libopencv_videoio.so.413"
    "libopencv_highgui.so.413"
    "libopencv_ml.so.413"
    "libopencv_dnn.so.413"
    "libopencv_features2d.so.413"
    "libopencv_flann.so.413"
    "libopencv_objdetect.so.413"
    "libopencv_photo.so.413"
    "libopencv_calib3d.so.413"
    "libopencv_stitching.so.413"
    "libopencv_gapi.so.413"
    "libopencv_video.so.413"
)

echo "Creating symlinks for OpenCV 4.13 libraries..."

for lib in "${REQUIRED_LIBS[@]}"; do
    # Extract base name (e.g., libopencv_core.so.413 -> libopencv_core.so)
    base_name=$(echo "$lib" | sed 's/\.413$//')
    
    # Try to find existing library (could be .4.1.3 or .4.5.4d)
    existing_lib=""
    if [ -f "$LIB_DIR/${base_name}.4.1.3" ]; then
        existing_lib="$LIB_DIR/${base_name}.4.1.3"
    elif [ -f "$LIB_DIR/${base_name}.4.5.4d" ]; then
        existing_lib="$LIB_DIR/${base_name}.4.5.4d"
    elif [ -f "$LIB_DIR/${base_name}.408" ]; then
        existing_lib="$LIB_DIR/${base_name}.408"
    fi
    
    if [ -n "$existing_lib" ]; then
        target="$LIB_DIR/$lib"
        if [ ! -e "$target" ]; then
            echo "Creating symlink: $target -> $(basename "$existing_lib")"
            ln -sf "$(basename "$existing_lib")" "$target"
        else
            echo "Symlink already exists: $target"
        fi
    else
        echo "Warning: Could not find library for $lib"
    fi
done

echo "Done! Updating library cache..."
ldconfig

echo "Verifying installation..."
python3 -c "import sys; sys.path = [p for p in sys.path if '/usr/local/lib/python3.10/dist-packages' not in p]; import cv2; print('OpenCV version:', cv2.__version__)" 2>&1

