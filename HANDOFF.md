# R200 → librealsense2 → ROS2 移植交接文档

> 日期：2026-08-13　主机：Ubuntu 22.04.5 (kernel 6.8.0-136-generic)　ROS2: Humble
> 相机：Intel RealSense LR200（`8086:0abf`，firmware `2.0.71.14`，serial `3211800941`）

## 1. 目录结构

```text
/home/zmiaow/r200_ros2/
├── upstream/
│   ├── librealsense/                 # RS1 v1.12.1 上游，分支 r200-rs1-compat
│   ├── librealsense2-v2.51.1/        # RS2 2.51.1 fork，分支 r200-rs2-port  ★核心
│   └── realsense-ros/                # ROS2 驱动 4.51.1，分支 r200-ros2
├── ros2_ws/                          # colcon 工作区（realsense2_camera 等）
├── rs2_install/                      # R200 版 librealsense2 安装前缀
├── tools/                            # RS1/RS2 探针与基准工具（git 仓库）
├── demo/                             # ROS2 演示包 r200_demo（launch/RViz/切换脚本）
└── docs/HANDOFF.md                   # 本文档
```

## 2. 架构结论（本项目的核心证据链）

- RS1 中 R200 与 SR300 共用 DS4 代码（`ds-private.cpp/h`）；其 XU 协议、
  SPI 标定解析、`stream_intent`、`modesLR[]` 已被整体移植到 RS2 的
  `src/r200/r200-private.*`、`src/r200/r200.*`，**没有逆向新协议**。
- RS2 侧 R200 被建模为 3 个 synthetic sensor（depth / IR / color），
  复用 `synthetic_sensor` + `uvc_sensor` + processing block 机制，与
  SR300（`src/ivcam/sr300.*`）同构。
- 深度原生 UVC 为 `628x469`，RS1 用 6px 边框 padding 暴露 `640x480`；
  RS2 通过 `r200_depth_pad` 处理块复刻该行为，intrinsics 取自标定
  `modesLR[0..2]`（640x480 / 492x372 / 332x252）。
- IR 原生为交错 `Y8I 640x481`，`r200_y8i_to_y8y8` 拆分左右并裁掉末行，
  暴露 `640x480` 的 IR1/IR2。

## 3. 已验证结果（与 RS1 baseline 对照）

| 项目 | 值 | 对照 |
| --- | --- | --- |
| serial / fw | `3211800941` / `2.0.71.14` | ✓ 与 RS1 一致 |
| depth scale | `0.001` m | ✓ |
| depth 640x480 intrinsics | `fx=fy=573.909, ppx=320.807, ppy=239.195` | ✓ |
| color 640x480 intrinsics | `fx=610.536, fy=610.589, ppx=326.145, ppy=234.774` | ✓ |
| color distortion | `0.0767581, -0.13888, 0.00160403, -0.000376356, 0` | ✓ |
| depth→color extrinsics | `trans=(-0.0577, 0.00182, -0.0004)` | ✓ |
| depth→IR2 baseline | `-0.06999` m | ✓（`rs2_pointcloud` 工具输出） |
| rs2::pointcloud | 307200 XYZ+RGB | ✓ |
| ROS2 话题 | depth/color/infra1/infra2/camera_info/points/TF | ✓ |

帧率（实测）：

- RS2 直连 depth-only 640x480@60：**≈56.6 Hz**
- ROS2 全链路（depth60 + color60 + IR30 + pointcloud）：**≈41 Hz**
  （处理开销，非固件上限；关掉 pointcloud 或减少流可提升）

## 4. 构建与运行

### RS2 fork

```bash
cd /home/zmiaow/r200_ros2/upstream/librealsense2-v2.51.1/build
cmake --build . -j$(nproc)
cmake --install . --prefix /home/zmiaow/r200_ros2/rs2_install
```

首次配置参数见仓库历史（BUILD_EXAMPLES=OFF 等）。

### ROS2 工作区

```bash
cd /home/zmiaow/r200_ros2/ros2_ws
source /opt/ros/humble/setup.bash
colcon build \
  --cmake-args \
  -Drealsense2_DIR=/home/zmiaow/r200_ros2/rs2_install/lib/cmake/realsense2 \
  -DBUILD_TESTING=OFF
```

### 运行 demo（Intel 官方 rs_launch.py 风格）

```bash
cd /home/zmiaow/r200_ros2/ros2_ws
source /opt/ros/humble/setup.bash
source install/setup.bash
export LD_LIBRARY_PATH=/home/zmiaow/r200_ros2/rs2_install/lib:$LD_LIBRARY_PATH

ros2 launch r200_demo r200_demo.launch.py depth_profile:=640x480x60
```

RViz2 显示 Depth / RGB / Infra1 / Infra2 四个图像 + RGB 点云
（`/camera/depth/color/points`，固定坐标系 `camera_link`）。

切换分辨率（重启式，最可靠）：

```bash
ros2 run r200_demo r200_switch.sh 640x480x90 1280x720x30
ros2 run r200_demo r200_switch.sh 640x480x30 640x480x30
```

## 5. 已知限制（诚实清单）

- **运行中切换 IR 会失败**：R200 固件不允许在其他流活动时重配置 IR
  接口（`VIDIOC_S_FMT → EIO`），只有重启节点/重插 USB 能恢复。因此
  切换采用“重启 launch”方式；depth/color 的运行中切换可用但不保证 IR
  不受影响。
- **元数据/timestamp**：当前 timestamp 域为 `SYSTEM_TIME`，
  realsense2_camera 会打 `Frame metadata isn't available` 警告；
  RS1 的帧计数解析尚未移植（后续可加 R200 metadata 解析器）。
- **RGB 点云纹理**：depth 60fps 与 color 60fps 混合时偶发
  `No stream match for pointcloud texture` 警告（时间戳对齐），
  全 30fps 时无此问题。
- **点云几何**：当前测试场景深度非常稀疏（~2% 有效像素，RS1 同场景
  一致），需要用实际场景目视复核 XYZ 几何。
- 中/低分辨率（492x372 / 332x252）的 profile 已暴露，但尚未逐一跑
  ROS2 端到端验证。

## 6. 硬件/环境注意

- **不要刷写 R200 固件**。
- 若 XU 全部返回 `ENOENT`（典型冷启动后）：物理重插 USB 即可恢复；
  官方 workaround 是对 uvcvideo 做 unbind/bind（`config/usb-R200-in`）。
- 节点是 USB3.0（Bus 002），带宽不是 60fps 的瓶颈；41Hz 属处理开销。
- 运行任何直连 RS2 工具前，先停掉 ROS2 节点（视频节点互斥）。

## 7. Git 状态与后续 fork 建议

| 仓库 | 分支 | 远端 | 要点 |
| --- | --- | --- | --- |
| `upstream/librealsense2-v2.51.1` | `r200-rs2-port` | 上游 2.51.1 | 所有 RS2 移植提交 |
| `upstream/librealsense-ros` | `r200-ros2` | 上游 4.51.1 | PID 白名单 + IR sensor 适配 |
| `upstream/librealsense` | `r200-rs1-compat` | 上游 v1.12.1 | RS1 兼容修复（仅基线用） |
| `tools/` | `master` | 本地 | 探针/基准工具 |

你的计划（fork 到自己仓库 → 推送当前更新 → 再试新版 RS2）建议顺序：

1. 在 GitHub fork `IntelRealSense/librealsense`（2.51.x 历史版本）后：
   `git remote add mine git@github.com:<you>/librealsense.git`
   `git push mine r200-rs2-port`
2. 同样处理 `realsense-ros`。
3. 新 RS2 兼容尝试：新版本删除了 SR300/R200 legacy 相关代码（`src/ivcam`
   等），需要先 diff 本分支 vs 上游 2.51.1 拿到最小补丁集，再针对新
   版 backend/sensor 接口逐项适配。重点接口变化：`synthetic_sensor`、
   `uvc_sensor`、processing block 的 `stream_resolution` 机制、metadata。
