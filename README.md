# R200 ROS2 兼容工程

本工程让 R200/LR200 通过 librealsense2 2.51.1 和官方
`realsense2_camera` ROS2 节点工作，并尽量让不依赖 D400 专属能力的上层
应用在 R200 与无 IMU 的 D435 之间复用。核心驱动保持通用、可审阅、可继续
向上游演进；GitHub 仓库采用公开 fork/公开配套仓库，代码不得依赖某台机器、
某个序列号或个人目录。

当前稳定基线：

- librealsense2 2.51.1（保留 SR300 legacy 的最后一代基线）
- realsense-ros 4.51.1
- ROS2 Humble / Linux
- 真机基线：LR200，固件 2.0.71.14

## 两个已确认预设

| 预设 | Depth / IR1 / IR2 | Color | 用途 |
| --- | --- | --- | --- |
| 默认吞吐 | 640x480@60 | 640x480@60 | 同频、低延迟、点云和实时算法 |
| 彩色质量 | 640x480@30 | 1920x1080@30 | 颜色细节优先 |

默认吞吐预设在当前真机上，开启 RGB 点云后四路图像约 59.5Hz、点云约
59.4Hz。质量预设能正确协商全部模式，但 ROS2 外部订阅 1080p RGB 时实测约
23-27Hz；深度、双红外和内部点云仍约 30Hz。因此默认选择 480p60，1080p30
保留为质量优先选项，不把它宣传成稳定满 30Hz 的实时预设。

## 构建

先构建并安装 R200 版 librealsense2，再构建 ROS2 工作区。路径使用变量，
不要求仓库位于特定用户目录。

```bash
export R200_ROOT=/path/to/r200_ros2

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

## 运行

```bash
source /opt/ros/humble/setup.bash
source "$R200_ROOT/ros2_ws/install/setup.bash"
export LD_LIBRARY_PATH="$R200_ROOT/rs2_install/lib:${LD_LIBRARY_PATH:-}"

# 默认：四路 640x480@60 + RGB 点云 + GUI
ros2 launch r200_demo r200_demo.launch.py

# 无 GUI 验收
ros2 launch r200_demo r200_demo.launch.py gui:=false

# 彩色质量预设
ros2 launch r200_demo r200_demo.launch.py \
  depth_profile:=640x480x30 \
  ir_profile:=640x480x30 \
  color_profile:=1920x1080x30
```

Launch 直接调用官方 `realsense2_camera_node`，使用官方参数名
`depth_module.profile`、`rgb_camera.profile`、`enable_sync`、
`pointcloud.enable` 等。R200 的红外是独立 UVC sensor，因此额外显式设置由
官方 wrapper 自动生成的 `stereo_ir_sensor.profile`；官方 4.51.1 的
`rs_launch.py` 没有声明这个参数。

R200 固件对活动状态下的部分 IR 重配置不稳，切换脚本采用完整停止后重启：

```bash
ros2 run r200_demo r200_switch.sh
ros2 run r200_demo r200_switch.sh 640x480x30 1920x1080x30
```

## 话题与功能对齐边界

通用功能：depth/color/infra1/infra2 图像、CameraInfo、内外参、TF、深度单位、
RGB PointCloud2、标准 profile 参数；R200 stereo 曝光/增益/自动曝光/发射器和
彩色 UVC 控件通过官方 wrapper 自动成为 ROS 参数。Y12I 已可在 RS2 SDK 中拆成
左右 Y16；realsense-ros 4.51.1 官方 profile manager 固定选择 Y8，Y16 的 ROS
格式参数将在新版官方 wrapper 迁移中按其 `infra1_format/infra2_format` 接口接入。

不伪装成 D435 的功能：D400 Advanced Mode、硬件同步、IMU、on-chip
calibration、可调激光功率。R200 只提供真实的发射器开关。上层应用应检测
capability，而不是仅按产品名分支。

## 验收与后续

- [ACCEPTANCE.md](ACCEPTANCE.md)：需求逐项映射、命令、证据与状态
- [DEMO_ENTRYPOINTS.md](DEMO_ENTRYPOINTS.md)：所有用户功能、命令和 ROS 话题入口
- [UPSTREAM_AND_LICENSE.md](UPSTREAM_AND_LICENSE.md)：官方基线、精确改动量和开源义务
- [HANDOFF.md](HANDOFF.md)：架构、仓库和维护交接
- [TODO.md](TODO.md)：按优先级排列的未完成项和完成定义
- librealsense 核心说明：`doc/r200-support.md`

## Codex 使用 Skill

仓库附带 [operate-r200-ros2](skills/operate-r200-ros2/SKILL.md) Skill。它让
Codex 或兼容 Agent 按固定流程完成环境检查、核心与 ROS2 构建、两种预设启动、
验收、控件诊断和发布维护，同时约束其不得伪造 R200 不具备的 D400 硬件能力。

在仓库中可直接指定 Skill 路径使用；需要安装到个人 Codex 环境时复制完整目录：

```bash
cp -a skills/operate-r200-ros2 "${CODEX_HOME:-$HOME/.codex}/skills/"
```

典型调用：`使用 $operate-r200-ros2 检查环境并启动默认四路 480p60 点云模式。`
Skill 的 `scripts/doctor.sh` 是只读诊断，不写固件、不重置 USB，也不修改系统配置。

在 `ACCEPTANCE.md` 的发布门禁全部满足前不推送正式发布分支。D435 真机和
一小时长稳当前缺少验收条件，会明确保留为待验，不能写成通过。

本配套仓库以 Apache-2.0 发布，顶层 `LICENSE` 已包含完整许可证；两个上游
fork 继续保留各自的 `LICENSE`、`NOTICE` 和第三方许可文本。
