---
name: operate-r200-ros2
description: Build, launch, verify, diagnose, or maintain the community Intel RealSense R200/LR200 librealsense2 and ROS 2 driver. Use for R200 ROS 2 setup, D435-compatible topic and parameter operation, realtime 480p60 or 1080p color presets, point-cloud checks, profile switching, hardware-option inspection, release acceptance, and minimal upstream-aligned driver changes.
---

# Operate R200 ROS 2

Operate the R200 through the official `realsense2_camera_node` interface. Preserve
the common D435 ROS surface; keep R200-only handling below the public ROS API.

## Establish the checkout

1. Locate the companion repository containing `ACCEPTANCE.md`, `TODO.md`, and
   `launch/r200_demo.launch.py`.
2. Resolve the workspace root from `R200_ROOT` when set. Otherwise accept a local
   checkout with sibling `upstream/librealsense2-v2.51.1`,
   `upstream/realsense-ros`, `rs2_install`, and `ros2_ws` directories.
3. Never bake usernames, absolute checkout paths, device serial numbers, tokens,
   or firmware writes into code or documentation.
4. Run `scripts/doctor.sh [workspace-root]` for a non-destructive environment and
   device check.

Read [references/ros-interface.md](references/ros-interface.md) before changing
profiles, ROS names, options, or capability claims. Read the repository-level
`ACCEPTANCE.md` and `TODO.md` before changing or publishing driver code.

## Select the workflow

### Build

Build and install the R200 librealsense fork first, then build the ROS workspace
against that exact installation:

```bash
cmake -S "$R200_ROOT/upstream/librealsense2-v2.51.1" \
      -B "$R200_ROOT/upstream/librealsense2-v2.51.1/build"
cmake --build "$R200_ROOT/upstream/librealsense2-v2.51.1/build" -j4
cmake --install "$R200_ROOT/upstream/librealsense2-v2.51.1/build" \
      --prefix "$R200_ROOT/rs2_install"

source /opt/ros/humble/setup.bash
cd "$R200_ROOT/ros2_ws"
colcon build --cmake-args \
  -Drealsense2_DIR="$R200_ROOT/rs2_install/lib/cmake/realsense2" \
  -DBUILD_TESTING=OFF
```

Stop when either build fails. Do not substitute a system librealsense and then
claim the R200 fork was tested.

### Prepare every ROS command

```bash
source /opt/ros/humble/setup.bash
source "$R200_ROOT/ros2_ws/install/setup.bash"
export LD_LIBRARY_PATH="$R200_ROOT/rs2_install/lib:${LD_LIBRARY_PATH:-}"
```

Confirm `ros2 pkg prefix realsense2_camera` and `ros2 pkg prefix r200_demo`
resolve inside the intended workspace before testing.

### Launch the D435-compatible common surface

Use the realtime preset unless the user explicitly prioritizes color detail:

```bash
# Depth, Color, Infra1, Infra2 at 640x480x60; RGB point cloud enabled
ros2 launch r200_demo r200_demo.launch.py gui:=false
```

Use the quality preset for the existing 1080p color requirement:

```bash
ros2 launch r200_demo r200_demo.launch.py gui:=false \
  depth_profile:=640x480x30 \
  ir_profile:=640x480x30 \
  color_profile:=1920x1080x30
```

Add `pointcloud:=false` only when the caller does not need PointCloud2. Set
`gui:=true` for the supplied RViz and four image windows.

### Switch profiles safely

Use a full node restart; do not change only one active R200 UVC sensor in place:

```bash
ros2 run r200_demo r200_switch.sh 640x480x30 1920x1080x30
```

The optional third argument selects IR; when omitted it follows depth.

### Verify

Verify the default preset with all four images and a non-empty RGB point cloud:

```bash
ros2 run r200_demo r200_acceptance.py \
  --duration 6 --require-pointcloud
```

Verify the quality preset without pretending its external RGB path is already a
stable 30 Hz path:

```bash
ros2 run r200_demo r200_acceptance.py \
  --duration 6 --fps 30 --width 640 --height 480 \
  --color-width 1920 --color-height 1080 --require-pointcloud
```

Record actual rates, dimensions, point count, command, hardware, and commit.
Label unrun hardware gates `PENDING` or `DEFERRED`, never `PASS`.

### Diagnose

Use this order:

1. Run the bundled doctor.
2. Check `ros2 topic list`, `ros2 param list /camera/camera`, and the camera log.
3. Run repository tools `rs2_probe`, `rs2_options`, `rs2_stream all`, and
   `rs2_restart` as appropriate.
4. Treat one system-time metadata warning as a known timestamp-domain limitation;
   do not relabel it as a hardware clock.
5. On `VIDIOC_S_FMT EIO`, stop all camera processes and reopen the complete sensor
   set. Keep depth units read-only at 0.001 m.

## Maintain or migrate

- Keep `realsense-ros` differences limited to discovery and sensor modeling.
- Implement device behavior as standard RS2 profiles, frames, metadata, and typed
  options in the librealsense fork.
- Reuse official ROS parameter and topic names. Prefer capability detection over
  product-name branches.
- Keep the 2.51.1 release line separate from latest-version migration.
- Do not advertise D400 Advanced Mode, hardware sync, IMU, on-chip calibration,
  adjustable laser power, or a hardware clock on R200.
- Preserve Apache-2.0 `LICENSE`, `NOTICE`, attribution, and prominent modification
  notices in changed upstream files.
- Re-run the repository acceptance matrix after functional changes. D435 hardware
  acceptance remains separate from compilation and R200 hardware acceptance.

