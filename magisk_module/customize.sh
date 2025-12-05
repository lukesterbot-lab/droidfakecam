#!/system/bin/sh
#
# DroidFakeCam Magisk Module Installation Script
#

SKIPUNZIP=1

# Print module info
ui_print "- Installing DroidFakeCam"
ui_print "- Virtual Camera Injection Module"
ui_print "- "

# Extract module files
ui_print "- Extracting module files..."
unzip -o "$ZIPFILE" -x 'META-INF/*' -d "$MODPATH" >&2

# Set permissions
ui_print "- Setting permissions..."
set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/service.sh" 0 0 0755
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755

# Create Camera1 directory on external storage
CAMERA1_DIR="/storage/emulated/0/DCIM/Camera1"
ui_print "- Creating Camera1 directory: $CAMERA1_DIR"
mkdir -p "$CAMERA1_DIR" 2>/dev/null || true

# Print completion message
ui_print "- "
ui_print "- Installation complete!"
ui_print "- "
ui_print "- IMPORTANT: Place your replacement video at:"
ui_print "-   $CAMERA1_DIR/virtual.mp4"
ui_print "- "
ui_print "- Install the DroidFakeCam app to configure settings"
ui_print "- Reboot to activate the module"
