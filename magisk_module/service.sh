#!/system/bin/sh
#
# DroidFakeCam Service Script
# Runs at late_start service mode
#

MODDIR=${0%/*}

# Wait for boot to complete
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 1
done

# Additional wait for system to stabilize
sleep 5

# Create Camera1 directory if it doesn't exist
CAMERA1_DIR="/storage/emulated/0/DCIM/Camera1"
if [ ! -d "$CAMERA1_DIR" ]; then
    mkdir -p "$CAMERA1_DIR" 2>/dev/null
    chmod 755 "$CAMERA1_DIR" 2>/dev/null
fi

# Set system property to indicate module is active
resetprop ro.droidfakecam.enabled true

# Log module status
log -t "DroidFakeCam" "Module service started"
log -t "DroidFakeCam" "Camera1 directory: $CAMERA1_DIR"

# Check for virtual.mp4
if [ -f "$CAMERA1_DIR/virtual.mp4" ]; then
    log -t "DroidFakeCam" "Found virtual.mp4 - ready for camera replacement"
else
    log -t "DroidFakeCam" "Warning: virtual.mp4 not found in $CAMERA1_DIR"
fi
