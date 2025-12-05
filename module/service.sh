#!/system/bin/sh
# DroidFakeCam Service Script
# Runs at late_start service mode

MODDIR=${0%/*}
VIRTUAL_CAM_DIR="/sdcard/DCIM/Camera1"

# Wait for storage to be ready
while [ ! -d "/sdcard" ]; do
    sleep 1
done

# Ensure virtual camera directory exists
mkdir -p "$VIRTUAL_CAM_DIR" 2>/dev/null

# Log module start
log -t DroidFakeCam "Service started, media directory: $VIRTUAL_CAM_DIR"
