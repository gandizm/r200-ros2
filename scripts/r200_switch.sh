#!/usr/bin/env bash
# Relaunch the R200 demo with a different resolution/framerate.
#
# The R200 firmware does not tolerate reconfiguring the IR interface while
# other streams are active, so switching is implemented as a clean relaunch
# of the camera node instead of a runtime parameter toggle.
#
# Usage:
#   r200_switch.sh                         # all four streams at 640x480x60
#   r200_switch.sh 640x480x30 1920x1080x30 # quality preset
set -euo pipefail

DEPTH="${1:-640x480x60}"
COLOR="${2:-640x480x60}"
IR="${3:-$DEPTH}"
LOG=/tmp/r200_demo.log

PACKAGE_PREFIX="$(ros2 pkg prefix r200_demo)"
WORKSPACE="${R200_ROS2_WS:-$(dirname "$(dirname "$PACKAGE_PREFIX")")}"
PROJECT_ROOT="$(dirname "$WORKSPACE")"
RS2_PREFIX="${R200_RS2_PREFIX:-$PROJECT_ROOT/rs2_install}"

echo ">>> stopping current demo"
pkill -f 'r200_demo.launch.py' 2>/dev/null || true
pkill -f 'realsense2_camera_node' 2>/dev/null || true
pkill -f 'rviz2.*r200.rviz' 2>/dev/null || true
sleep 2

echo ">>> starting demo: depth=${DEPTH} ir=${IR} color=${COLOR}"
cd "$WORKSPACE"
export LD_LIBRARY_PATH="$RS2_PREFIX/lib:${LD_LIBRARY_PATH:-}"
nohup ros2 launch r200_demo r200_demo.launch.py \
    depth_profile:="$DEPTH" ir_profile:="$IR" color_profile:="$COLOR" \
    >"$LOG" 2>&1 &

sleep 4
echo ">>> demo relaunched, log: $LOG"
echo ">>> check rates: ros2 topic hz /camera/depth/image_rect_raw"
