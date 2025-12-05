#!/system/bin/sh
#
# DroidFakeCam Post-FS-Data Script
# Runs before zygote and most boot processes
#

MODDIR=${0%/*}

# Set early system property
resetprop ro.droidfakecam.version "1.0.0"

# Log initialization
log -t "DroidFakeCam" "Post-fs-data initialized"
