#!/usr/bin/env bash
# Relaunch the R200 demo with a different resolution/framerate.
#
# The R200 firmware does not tolerate reconfiguring the IR interface while
# other streams are active, so switching is implemented as a clean relaunch
# of the camera node instead of a runtime parameter toggle.
#
# Usage:
#   r200_switch.sh                      # default 640x480x60 / 640x480x60
#   r200_switch.sh 640x480x30 640x480x30
#   r200_switch.sh 640x480x90 1280x720x30
set -euo pipefail

WORKSPACE=/home/zmiaow/r200_ros2/ros2_ws
RS2_PREFIX=/home/zmiaow/r200_ros2/rs2_install
DEPTH="${1:-640x480x60}"
COLOR="${2:-640x480x60}"
LOG=/tmp/r200_demo.log

echo ">>> stopping current demo"
pkill -f 'r200_demo.launch.py' 2>/dev/null || true
pkill -f 'realsense2_camera_node' 2>/dev/null || true
pkill -f 'rviz2.*r200.rviz' 2>/dev/null || true
sleep 2

echo ">>> starting demo: depth=${DEPTH} color=${COLOR}"
cd "$WORKSPACE"
source /opt/ros/humble/setup.bash
source install/setup.bash
export LD_LIBRARY_PATH="$RS2_PREFIX/lib:$LD_LIBRARY_PATH"
nohup ros2 launch r200_demo r200_demo.launch.py \
    depth_profile:="$DEPTH" color_profile:="$COLOR" \
    >"$LOG" 2>&1 &

sleep 4
echo ">>> demo relaunched, log: $LOG"
echo ">>> check rates: ros2 topic hz /camera/depth/image_rect_raw"
