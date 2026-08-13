#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Non-destructive environment check for the community R200 ROS 2 stack.
set -u

root="${1:-${R200_ROOT:-}}"
failures=0

pass() { printf 'PASS  %s\n' "$*"; }
warn() { printf 'WARN  %s\n' "$*"; }
fail() { printf 'FAIL  %s\n' "$*"; failures=$((failures + 1)); }

if [[ -z "$root" ]]; then
  script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  candidate="$(cd "$script_dir/../../../.." && pwd)"
  if [[ -d "$candidate/ros2_ws" ]]; then
    root="$candidate"
  else
    fail 'set R200_ROOT or pass the workspace root as argument 1'
  fi
fi

if [[ -n "$root" ]]; then
  [[ -d "$root/upstream/librealsense2-v2.51.1" ]] \
    && pass 'R200 librealsense source found' \
    || fail 'missing upstream/librealsense2-v2.51.1'
  [[ -d "$root/upstream/realsense-ros" ]] \
    && pass 'realsense-ros source found' \
    || fail 'missing upstream/realsense-ros'
  [[ -f "$root/rs2_install/lib/cmake/realsense2/realsense2Config.cmake" ]] \
    && pass 'R200 librealsense install found' \
    || fail 'missing rs2_install librealsense CMake package'
  [[ -f "$root/ros2_ws/install/setup.bash" ]] \
    && pass 'ROS 2 workspace install found' \
    || fail 'missing ros2_ws/install/setup.bash'
fi

[[ -f /opt/ros/humble/setup.bash ]] \
  && pass 'ROS 2 Humble found' \
  || fail 'ROS 2 Humble setup not found'

if command -v lsusb >/dev/null 2>&1; then
  devices="$(lsusb 2>/dev/null)"
  if grep -Eqi '8086:(0a80|0abf)' <<<"$devices"; then
    pass 'R200/LR200 USB device detected'
  else
    warn 'no R200/LR200 USB device detected (expected 8086:0a80 or 8086:0abf)'
  fi
else
  warn 'lsusb is unavailable; USB detection skipped'
fi

if pgrep -af 'realsense2_camera_node|r200_demo.launch.py' >/dev/null 2>&1; then
  warn 'a camera node or demo launch is already running'
else
  pass 'no existing R200 ROS process detected'
fi

if (( failures > 0 )); then
  printf '\nDoctor result: FAIL (%d blocking issue(s))\n' "$failures"
  exit 1
fi

printf '\nDoctor result: PASS (warnings may still require attention)\n'
