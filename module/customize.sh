#!/system/bin/sh
# DroidFakeCam Magisk Module Installation Script
# For educational and research purposes only

SKIPUNZIP=1

# Extract module files
ui_print "- Extracting module files"
unzip -o "$ZIPFILE" -d "$MODPATH" >&2

# Set permissions
ui_print "- Setting permissions"
set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/service.sh" 0 0 0755

# Zygisk library permissions
if [ -d "$MODPATH/zygisk" ]; then
    set_perm_recursive "$MODPATH/zygisk" 0 0 0755 0644
    for lib in "$MODPATH/zygisk"/*.so; do
        [ -f "$lib" ] && set_perm "$lib" 0 0 0644
    done
fi

# Create virtual camera directory on sdcard
ui_print "- Creating virtual camera directory"
VIRTUAL_CAM_DIR="/sdcard/DCIM/Camera1"
mkdir -p "$VIRTUAL_CAM_DIR" 2>/dev/null || true

# Info message
ui_print ""
ui_print "=========================================="
ui_print " DroidFakeCam installed successfully!"
ui_print "=========================================="
ui_print ""
ui_print " Place your media files in:"
ui_print "   $VIRTUAL_CAM_DIR"
ui_print ""
ui_print " Supported files:"
ui_print "   - virtual.mp4 (video feed)"
ui_print "   - 1000.bmp (photo capture)"
ui_print ""
ui_print " Control files (create empty files):"
ui_print "   - disable.jpg (disable hooking)"
ui_print "   - no_toast.jpg (suppress logs)"
ui_print "   - private_dir.jpg (use app-specific dirs)"
ui_print ""
ui_print " Check logcat for debug info:"
ui_print "   adb logcat -s DroidFakeCam"
ui_print ""
ui_print " NOTE: Requires Zygisk to be enabled in Magisk"
ui_print "=========================================="
ui_print ""
