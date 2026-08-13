# ROS 2 compatibility contract

## Common D435-facing surface

The companion launch runs the official `realsense2_camera_node`; it does not
define a parallel R200 ROS driver. Applications should consume these standard
interfaces:

| Function | Interface |
| --- | --- |
| Depth image | `/camera/depth/image_rect_raw` |
| Color image | `/camera/color/image_raw` |
| Left/right infrared | `/camera/infra1/image_raw`, `/camera/infra2/image_raw` |
| RGB point cloud | `/camera/depth/color/points` |
| Calibration | Per-stream `camera_info` topics |
| Transforms | `/tf`, `/tf_static` |
| Profiles | `depth_module.profile`, `stereo_ir_sensor.profile`, `rgb_camera.profile` |
| Synchronization | `enable_sync` |
| Point cloud | `pointcloud.enable` |

R200 stereo exposure, gain, auto exposure, and emitter state are standard RS2
typed options and therefore become `depth_module.*` parameters through the
official wrapper. Supported color UVC controls similarly become `rgb_camera.*`.
Query the running node instead of assuming every optional parameter exists.

## Presets

| Preset | Depth / IR1 / IR2 | Color | Expected use |
| --- | --- | --- | --- |
| Realtime | 640x480@60 | 640x480@60 | Default, common-rate processing and point clouds |
| Color quality | 640x480@30 | 1920x1080@30 | Higher color detail |

The quality profile negotiates correctly. Existing acceptance measured the
externally observed 1080p RGB path at roughly 23-27 Hz under ROS load, so report
it as usable but not yet a guaranteed 30 Hz external delivery path.

## Intentional differences

- R200 exposes a third, independent stereo IR UVC sensor, hence the explicit
  `stereo_ir_sensor.profile` launch parameter.
- Depth units are calibrated at 0.001 m and remain read-only. Making the option
  writable caused deterministic IR startup failure in the official ROS sensor
  startup sequence.
- Y12I is split to left/right Y16 in the RS2 core. ROS wrapper 4.51.1 selects Y8;
  use the latest wrapper's official per-stream format parameters during migration.
- Profile changes require a complete node restart because partial live R200
  reconfiguration is not reliable.
- Frame counters are real embedded metadata; timestamps honestly use
  `SYSTEM_TIME` because no proven per-frame R200 hardware clock is available.

## Unsupported hardware claims

Do not emulate or advertise D400 Advanced Mode, on-chip calibration, hardware
sync, IMU, adjustable laser power, or a hardware-clock timestamp domain. These
are hardware capabilities, not ROS API compatibility requirements. Applications
must test capabilities with `supports(...)` or the exposed ROS parameters.

## Acceptance thresholds

- Default preset: four 640x480 streams requested at 60 Hz; acceptance minimum is
  80% of requested rate; RGB point cloud must be non-empty and at least 20 Hz.
- Quality preset: verify negotiated dimensions and record actual rates rather
  than claiming an unmeasured 30 Hz.
- Preserve `PENDING` for D435 hardware regression until a D435 is physically run
  against the same build.

