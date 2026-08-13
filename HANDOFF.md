# R200 → librealsense2 → ROS2 交接

更新时间：2026-08-13。稳定化基线为 librealsense2 2.51.1 和
realsense-ros 4.51.1；2.58.3 仅用于第二阶段差异评估，尚未移植。

## 仓库角色

| 目录 | 分支 | 角色 |
| --- | --- | --- |
| `upstream/librealsense` | `r200-rs1-compat` | 官方 RS1 1.12.1 行为证据和基线探针 |
| `upstream/librealsense2-v2.51.1` | `r200-rs2-port` | 当前 R200 核心稳定分支 |
| `upstream/realsense-ros` | `r200-ros2` | ROS2 4.51.1 最小设备兼容修改 |
| `upstream/librealsense2` | 上游 2.58.3 快照 | 第二阶段迁移目标，不承载当前稳定修改 |
| `demo/validation` | `master` | RS1/RS2 探针、点云、速率和重启验收 |
| `demo` | `master` | ROS2 launch、RViz、Skill、切换脚本、验收和文档 |

不要把这些工作树直接合并成一团提交。librealsense 和 realsense-ros 使用官方
公开 fork，demo、validation 和 Skill 使用同一个公开配套仓库；每个 fork 保留
官方 `upstream`。

当前本地验收点：

| 仓库 | Commit | 内容 |
| --- | --- | --- |
| RS1 | `29e2fe0` | Ubuntu 22.04 基线兼容 |
| RS2 core | `aff037f` | 模式/帧序/重启、typed controls、双 Y16、支持及变更标记 |
| realsense-ros | `b02bc1e` | PID/video sensor 最小兼容及 Apache-2.0 变更标记 |
| tools | `526a32b` | counter、重启、profile、controls 与双 Y16 验收 |
| demo | `4434cf4` | 同频预设、公开发布、功能入口、验收和 TODO 文档 |

以上 commit 均为本地提交；GitHub remote、release tag 和发布 commit 仍需在用户
登录并确认仓库名后记录。

## 架构

R200 被建模为一个 RS2 device、三个 synthetic sensor：

```text
R200/LR200 USB device
├── Stereo Module      -> raw Z16 -> crop or 6px zero-border -> Depth
├── Stereo IR Sensor   -> raw Y8I/Y12I -> left/right Y8/Y16  -> Infra1/Infra2
└── RGB Camera         -> raw YUYV -> standard converters    -> Color
```

校准和 DS4 extension-unit 协议来自官方 RS1 `ds-private.*` / `ds-device.cpp`。
设备对外使用正常 RS2 stream/profile/intrinsics/extrinsics/metadata/matcher API，
ROS wrapper 不需要一套平行的 R200 消息协议。

### 有效图像与传输缓冲

DS4 的 depth/IR UVC 缓冲包含 metadata 行和未使用列。正确用户尺寸为
640x480 / 628x468、492x372 / 480x360、332x252 / 320x240；旧实现暴露的
628x469、628x361、628x242 是传输尺寸，不是有效图像尺寸。

Rectified profile 只添加六像素零边框，不做插值；unpadded profile 删除边框，
主点坐标减六，焦距不变。最终 dinghy 行仅用于 frame counter。

### 帧匹配与时间

- depth/IR：读取 dinghy `frameCount`
- YUYV color：读取最后 32 个像素的 LSB 编码
- depth + IR1 + IR2：frame-number matcher
- color 与双目组：backend timestamp matcher
- domain：`SYSTEM_TIME`，不是伪造的 hardware clock

`src/sensor.cpp` 的通用小改动只在 timestamp reader 运行期间向 frame 暂时暴露
backend payload 指针，随后仍由原 backend continuation 管理生命周期。该改动
必须在 D435/SR300 上回归。

### stream-intent

三个 sensor 第一次 open 前写保守全 mask 0x7。每个成功 open 增加计数；全部
close 后清除 written 标志，下一完整周期重新写入。`validation/build/rs2_restart`
已在同一
设备对象内验证两轮。部分 sensor 在线重配和准确 active mask 仍在 TODO。

## ROS2 最小差异

realsense-ros 分支只做两类核心兼容：

1. 识别 R200/LR200 PID；
2. 视频 sensor 的判断接受任何包含 video profile 的 sensor，使独立 R200 IR
   sensor 进入官方 profile/publisher 路径。

Demo 直接运行官方 `realsense2_camera_node`，沿用官方话题、参数、TF 和
pointcloud filter。唯一 R200 launch 特例是显式传
`stereo_ir_sensor.profile`，因为它是第三个独立 sensor，而官方 4.51.1
`rs_launch.py` 未声明这个动态模块参数。

Stereo exposure/gain/auto-exposure/emitter 及 RGB UVC controls 都由 core 注册为
标准 RS2 option，因此自动进入官方 ROS 动态参数路径。Depth units 保持只读
0.001m；可写候选会导致 4.51.1 按 sensor 顺序启动时 IR `VIDIOC_S_FMT EIO`。

## 已验证与未验证

详细数字见 [ACCEPTANCE.md](ACCEPTANCE.md)。摘要：

- core/ROS2 构建：通过
- RS1/RS2 模式白名单、标定、外参、四路 frameset/counter：通过
- 同一 device 两次完整启停：通过
- 默认四路 480p60 + RGB 点云：约 59.4-59.5Hz，通过
- 1080p30 质量 profile：协商通过；外部 RGB 约 23-27Hz，性能部分通过
- 六组 depth/IR 尺寸 × 30/60/90fps：18/18 通过
- 六组 Y12I→左右 Y16 @30：6/6 通过（ROS 4.51.1 仍固定选择 Y8）
- typed options 的 ROS query/set/restore 和恢复后四路：通过
- 一小时长稳、热插拔：用户决定本阶段延期
- D435 真机 common-path 回归：待验（当前未连接 D435）

## GitHub 发布方案

目标是全部公开并保持通用。两个上游项目创建公开 fork：

```bash
git remote rename origin upstream
git remote add origin git@github.com:ACCOUNT/FORK.git
git push -u origin r200-rs2-port
```

实际仓库名和账号必须在用户登录后确认。不得在文档、remote URL 输出或日志中
保存 token。推送前执行验收矩阵 P5-P8，并记录 commit/tag。

## 第二阶段迁移

2.58.3 不能机械 cherry-pick：设备发现、backend/profile/metadata 架构已经
变化，而且 2.51.1 R200 使用的 product-line mask 0x20 在 2.58.3 已分配给
D500。迁移顺序必须是：

1. 冻结并标记 2.51.1 稳定点；
2. 建 migration 分支；
3. 先解决公开 product-line bit 和设备发现；
4. 移植 DS4/calibration 与三个 sensor；
5. 适配 processing/metadata/matcher；
6. 对齐最新版官方 ROS2 参数和 launch；
7. 重跑完整验收并与 2.51.1 比较。

若迁移失败，不影响 2.51.1 稳定分支和已发布 tag。
