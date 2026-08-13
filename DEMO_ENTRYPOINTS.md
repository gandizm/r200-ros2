# 功能与 Demo 入口

先 source ROS2 工作区并让运行时找到本项目的 librealsense：

```bash
source /opt/ros/humble/setup.bash
source /path/to/r200_ros2/ros2_ws/install/setup.bash
export LD_LIBRARY_PATH=/path/to/r200_ros2/rs2_install/lib:${LD_LIBRARY_PATH:-}
```

## 日常使用入口

| 功能 | 命令或入口 | 默认输出 |
| --- | --- | --- |
| 完整 GUI demo | `ros2 launch r200_demo r200_demo.launch.py` | 四路 480p60、RGB 点云、RViz、图像窗口 |
| 无 GUI 节点 | `ros2 launch r200_demo r200_demo.launch.py gui:=false` | 同上，不打开窗口 |
| 1080p 彩色质量模式 | 见下方命令 | Depth/IR 480p30、Color 1080p30、点云 |
| 关闭点云 | launch 追加 `pointcloud:=false` | 四路图像和 CameraInfo |
| 完整重启切换 profile | `ros2 run r200_demo r200_switch.sh [DEPTH] [COLOR] [IR]` | 停止旧节点后用新 profile 重启 |
| 自动验收 | `ros2 run r200_demo r200_acceptance.py --duration 6 --require-pointcloud` | 检查尺寸、帧率和非空点云 |

1080p 彩色质量模式：

```bash
ros2 launch r200_demo r200_demo.launch.py \
  depth_profile:=640x480x30 \
  ir_profile:=640x480x30 \
  color_profile:=1920x1080x30
```

切换脚本的参数顺序是 depth、color、IR；省略 IR 时跟随 depth：

```bash
ros2 run r200_demo r200_switch.sh 640x480x30 1920x1080x30
```

## ROS 2 功能入口

| 功能 | 入口 |
| --- | --- |
| 深度图 | `/camera/depth/image_rect_raw` |
| 彩色图 | `/camera/color/image_raw` |
| 左/右红外 | `/camera/infra1/image_raw`、`/camera/infra2/image_raw` |
| RGB 点云 | `/camera/depth/color/points` |
| 相机参数 | 各图像同命名空间下的 `camera_info` |
| 坐标变换 | `/tf`、`/tf_static` |
| profile | `depth_module.profile`、`stereo_ir_sensor.profile`、`rgb_camera.profile` |
| 同步 | `enable_sync` |
| 点云开关 | `pointcloud.enable`；本 demo 的 launch 参数名为 `pointcloud` |

查看当前节点暴露的全部官方参数与本机话题：

```bash
ros2 param list /camera/camera
ros2 topic list | sort
ros2 topic info /camera/depth/color/points
```

曝光、增益、自动曝光和发射器已由官方 wrapper 自动生成 ROS 参数。R200 的
depth units 固定为已标定的只读 0.001m，因此官方 4.51.1 不生成动态参数；深度
帧和点云仍使用该真实单位。

常用动态控制示例：

```bash
ros2 param set /camera/camera depth_module.enable_auto_exposure false
ros2 param set /camera/camera depth_module.exposure 16300.0
ros2 param set /camera/camera depth_module.gain 400
ros2 param set /camera/camera depth_module.emitter_enabled true
ros2 param set /camera/camera rgb_camera.enable_auto_exposure true
```

Stereo exposure 范围随 30/60/90fps 改变；以 `rs2_options` 和实际 set 结果为准。
手动设置 RGB exposure/gain 会在硬件上关闭 color AE，设置 white balance 会关闭
AWB；ROS 4.51.1 的关联 bool 参数值不会因这个硬件副作用自动刷新，重新查询 SDK
或显式设置 bool 参数可得到/保持期望状态。

## 底层诊断和验收入口

这些程序位于 `tools/`，用于驱动验收，不是日常 ROS 节点：

| 程序 | 作用 |
| --- | --- |
| `rs1_probe` | 记录官方 RS1 的 R200 profile 基线 |
| `rs1_baseline` | 记录 RS1 的标定、内外参和 stream 基线 |
| `rs1_controls` | 验证官方 RS1 的彩色手动/自动控件联动基线 |
| `rs2_probe` | 枚举 RS2 设备、sensor、profile、内外参和 option |
| `rs2_options` | 枚举每个 sensor 的 typed option、当前值、范围和只读状态 |
| `rs2_y16` | 验收 Y12I 拆包后的左右 Y16 尺寸、stride、像素范围和重启 |
| `rs2_stream all` | 打开 depth/color/IR1/IR2，检查帧、计数和时间戳 |
| `rs2_pointcloud` | 直接用 RS2 SDK 生成 RGB 点云 |
| `rs2_rate` | 检查 480p60 depth 实际速率 |
| `rs2_restart` | 同一对象连续两轮完整 open/start/stop/close |
| `rs2_profile_matrix` | 跑六种 depth/IR 尺寸 × 30/60/90，共 18 组 |
| `xu_test` | 低层 XU 协议诊断；非日常入口，错误写入可能影响设备状态 |

每个发布版本以 `ACCEPTANCE.md` 记录的命令和实际输出为准；仅出现某个入口，
不等于该项已经通过真机验收。

## Agent / Codex Skill 入口

配套仓库提供 `skills/operate-r200-ros2/SKILL.md`，用于让 Codex 或兼容 Agent
遵循同一套构建、启动、验收和诊断流程。Skill 的只读环境检查可直接执行：

```bash
skills/operate-r200-ros2/scripts/doctor.sh /path/to/r200_ros2
```

Skill 明确以官方 `realsense2_camera_node` 的通用接口作为 D435 兼容面，不创建
R200 私有 ROS 消息或平行节点，也不会把缺失硬件能力伪装成 D435 功能。
