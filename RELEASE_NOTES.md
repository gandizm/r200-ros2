# R200 ROS 2 v0.1.0-alpha.1

First public experimental release of the community Intel RealSense R200/LR200
port to librealsense2 and ROS 2.

## Pinned components

- `gandizm/librealsense`, tag `r200-v2.51.1-alpha.1`: official librealsense
  v2.51.1 plus the R200 RS2 core port;
- `gandizm/realsense-ros`, tag `r200-ros-v4.51.1-alpha.1`: official
  realsense-ros 4.51.1 plus the minimal R200 compatibility patch;
- `gandizm/r200-ros2`, tag `v0.1.0-alpha.1`: ROS launch, RViz, Codex Skill,
  validation tools, documentation, and acceptance evidence.

## Validated on LR200

- Depth, Color, Infra1, and Infra2 at 640x480@60, approximately 59.5 Hz;
- RGB PointCloud2, CameraInfo, intrinsics/extrinsics, and TF;
- quality preset with Depth/IR at 640x480@30 and Color at 1920x1080@30;
- standard stereo and color exposure/gain/auto controls and emitter state;
- 18/18 depth/IR profile combinations across 30/60/90 fps;
- Y12I to dual Y16 conversion at six resolutions in the RS2 SDK;
- complete stop/reopen and two-cycle device restart;
- core, ROS package, Skill, and validation-program builds.

## Known limitations

- The externally observed 1080p ROS color path is approximately 23-27 Hz under
  the validated workload, not a guaranteed full 30 Hz delivery path.
- realsense-ros 4.51.1 selects Y8 for IR topics. ROS `mono16` selection is planned
  through the latest wrapper's official per-stream format parameters.
- R200 uses honest `SYSTEM_TIME`; no hardware-clock timestamp is claimed.
- Profile switching uses a full node restart.
- One-hour stability, hot-plug recovery, multi-camera operation, and D435 hardware
  regression are deferred or pending.
- R200 does not provide D400 Advanced Mode, IMU, hardware synchronization,
  on-chip calibration, or adjustable laser power.

This is a community port, not an Intel-supported R200 release. See `README.md`,
`ACCEPTANCE.md`, `TODO.md`, and `UPSTREAM_AND_LICENSE.md` before deployment.
