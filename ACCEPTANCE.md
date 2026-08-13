# 验收矩阵

状态定义：`PASS` 已有可复现证据；`PARTIAL` 功能可用但未达到完整指标；
`PENDING` 缺少硬件、时间或用户授权；`BLOCKED` 已确认外部阻塞。

本文不把“能编译”当成“驱动验收通过”，也不把未连接的 D435 写成已验证。

## 用户确认与发布门禁

| ID | 要求 | 验收标准 | 当前状态 |
| --- | --- | --- | --- |
| R1 | GitHub 使用 Private | 用户登录；创建 Private 独立镜像；`upstream` 指向官方；验证可见性 | PENDING：等待用户登录 |
| R2 | 私有托管但代码通用 | 无用户名、绝对工程路径、序列号或机器判断；保留 Apache-2.0；文档可独立构建 | PASS（当前改动） |
| R3 | 先稳定 2.51.1 | 核心/ROS2 构建、真机模式、四路流、点云、重启、文档门禁完成 | PARTIAL：长稳和 D435 待验 |
| R4 | 再迁移最新版 | 2.51.1 发布点冻结后，单独分支移植 2.58.3；不得覆盖稳定分支 | PENDING |
| R5 | 默认四路 640x480@60 | ROS 参数全部为 60；每路尺寸正确；实测不低于 48Hz | PASS：四路约 59.5Hz |
| R6 | 质量预设 1080p30 | Depth/IR 480p30、Color 1080p30 能启动；记录实际外部发布率 | PARTIAL：模式通过，RGB 外部订阅约 23-27Hz |
| R7 | R200 与无 IMU D435 功能对齐 | 共用标准图像/CameraInfo/TF/点云/参数；D400 专属能力采用 capability 检测 | PARTIAL：代码边界已定义，D435 真机待验 |
| R8 | 基于 RS1、最小修改 | 模式/协议有 RS1 证据；改动集中；每个提交单一职责；无无关格式化 | PASS（未推送提交待整理） |
| R9 | ROS2 尽量对齐官方 | 使用官方 package/executable/参数和话题；R200 特例必须有原因 | PASS |
| R10 | 文档和 TODO 完整 | README、核心支持说明、交接、验收矩阵、TODO 均存在且相互一致 | PASS |
| R11 | 操作前讲懂并确认 | 架构或范围发生变化时先说明；GitHub 登录/建库/推送前再次确认目标名 | 持续门禁 |

GitHub 的真正 Fork 仓库不能设为 Private，因此 R1 的实现方式是“Private
独立镜像 + 官方 `upstream` remote”，不是公开 fork。这样同时满足私密管理
和后续跟随上游。

## 已完成真机验收

参考设备：LR200 `8086:0abf`，序列号仅作为本次证据记录，不进入驱动逻辑；
固件 2.0.71.14，Ubuntu 22.04 / ROS2 Humble。

### A1：RS1 模式基线

`tools/rs1_probe` 确认 depth/IR 用户可见尺寸为：

- 640x480、628x468、492x372、480x360、332x252、320x240
- 每种尺寸均支持 30/60/90 fps

颜色白名单与 librealsense 1.12.1 的 DS4 模式表一致。RS2 不得把
628x469、628x361、628x242 的传输缓冲尺寸冒充为图像尺寸。

### A2：RS2 profile 与标定

```bash
tools/rs2_probe
tools/rs2_stream all
```

通过条件与结果：

- 只暴露六组正确 depth/IR 尺寸和 RS1 颜色白名单：PASS
- 640x480 depth scale = 0.001m：PASS
- depth/color intrinsics 与 RS1 baseline 一致：PASS
- depth→color、depth→IR2 extrinsics 与 RS1 baseline 一致：PASS
- 四路 frameset 同时包含 depth/color/IR1/IR2：PASS
- 四路都暴露嵌入 frame counter，样本 counter 同为 2：PASS
- timestamp domain 如实为 `SYSTEM_TIME`：PASS

### A3：同一设备对象的完整重启

```bash
tools/rs2_restart
```

通过条件：同一个 device/sensor 对象连续执行两次
open→start→stop→close；两轮均收到四路帧；第二轮前日志再次出现
`R200 stream-intent=0x7`。

结果：两轮均 `PASS`，每轮 depth≥3、IR 合计≥6、color≥2。

### A4：默认 ROS2 全功能预设

```bash
ros2 launch r200_demo r200_demo.launch.py gui:=false
ros2 run r200_demo r200_acceptance.py --duration 6 --require-pointcloud
```

实测：

| 输出 | 请求 | 实测 | 状态 |
| --- | --- | --- | --- |
| depth | 640x480@60 | 59.54Hz | PASS |
| color | 640x480@60 | 59.54Hz | PASS |
| infra1 | 640x480@60 | 59.54Hz | PASS |
| infra2 | 640x480@60 | 59.54Hz | PASS |
| RGB pointcloud | 同步输出 | 59.37Hz，138001 个有效点样本 | PASS |

`r200_acceptance.py` 的默认阈值是请求帧率的 80%，同时检查四路尺寸；点云
启用时要求非空且至少 20Hz。

### A5：彩色质量预设

```bash
ros2 launch r200_demo r200_demo.launch.py gui:=false \
  depth_profile:=640x480x30 ir_profile:=640x480x30 \
  color_profile:=1920x1080x30
```

协商结果：四个请求 profile 全部正确打开。带点云和外部 Python 验收器时，
depth/IR 约 29.97Hz，点云约 29.76Hz，1080p RGB 约 23.11Hz；单独
`ros2 topic hz` 观察到约 24-27Hz。结论是“质量模式可用，但外部 RGB 发布
未稳定满 30Hz”，所以默认仍采用四路 480p60。

### A6：完整 depth/双红外模式矩阵

```bash
tools/rs2_profile_matrix
```

六组有效尺寸分别在 30/60/90fps 下执行 open/start/收帧/stop/close，共 18
个组合。逐帧检查输出尺寸、stride、buffer 大小、指针、metadata counter 与
frame number 一致性；每组至少收到 2 个 depth 和 4 个 IR 输出帧。

结果：`MATRIX total=18 failures=0 result=PASS`。

## 发布前仍需完成

| ID | 验收项 | 完成定义 | 状态 |
| --- | --- | --- | --- |
| P1 | 一小时默认预设长稳 | 60 分钟；无断流/EIO；四路与点云平均≥48Hz；记录丢帧和内存 | PENDING |
| P2 | 全模式抽样 | 六组 depth/IR 尺寸各跑 30/60/90；检查尺寸、stride、buffer 和 frame counter | PASS：18/18 |
| P3 | 热插拔恢复 | 节点运行→拔出→插入；在限定时间内恢复且无需刷固件 | PENDING |
| P4 | D435 回归 | 同一 RS2/ROS2 构建连接无 IMU D435；默认官方 profile、TF、点云不回归 | PENDING：当前无 D435 |
| P5 | 代码静态门禁 | `git diff --check`、core 和 ROS2 构建、Python/shell 语法通过 | PASS |
| P6 | Git 历史 | core、ROS2、demo/tools 按职责拆分提交；工作树干净；tag/commit 写入交接文档 | PARTIAL：提交/工作树通过，发布 tag 待建 |
| P7 | GitHub Private | 用户登录后创建目标仓库、推送、拉取验证、检查无凭据/大文件/构建物 | PENDING |

P1-P4 若用户决定延后，首次仓库版本必须标为 pre-release/experimental，且
TODO 不得删除这些项目。
