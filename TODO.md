# TODO

每项必须包含完成定义和证据，不以“AI 已实现”作为完成依据。优先级内按推荐
顺序执行。

## P0：2.51.1 首次公开版本

- [ ] （延期，不阻塞当前发布）完成 60 分钟默认预设长稳。
  - 完成定义：四路 640x480@60 + RGB 点云连续运行；无 EIO/断流；平均
    ≥48Hz；保存 CPU、RSS、丢帧与末尾日志。
- [x] 跑完六组 depth/IR profile 的 30/60/90 抽样矩阵。
  - 完成定义：尺寸、stride、buffer、frame counter 和 start/stop 均通过；
    传输 metadata 行不出现在图像中。
  - 证据：`tools/rs2_profile_matrix`，18/18 PASS；已检查尺寸、stride、buffer、
    counter 和完整启停。像素质量/数据范围仍由后续场景化测试覆盖。
- [x] 整理最小提交。
  - core：RS1 模式/裁剪/帧计数；stream-intent 生命周期；文档各自提交。
  - ROS2：只保留 PID/视频 sensor 兼容的必要提交。
  - demo/tools：launch、验收器和文档独立提交。
- [ ] 用户登录后创建 GitHub 公开 fork 和公开配套仓库。
  - librealsense、realsense-ros 使用官方公开 fork；添加官方仓库为 `upstream`。
  - 推送前检查无 token、SSH key、设备序列号逻辑、绝对个人路径、build/install
    产物和超大日志。
- [ ] 为首次版本打 pre-release 标签并附验收表。
  - 若 D435/热插拔/长稳仍未跑完，release notes 必须明确 experimental。

## P1：核心驱动完整性

- [x] 实现 D435 共通的 R200 typed options。
  - Stereo 曝光、增益、自动曝光、发射器已按 RS1 XU 范围和当前 FPS discovery
    实现；彩色 UVC 控件由设备逐项探测后注册。
  - ROS 已完成 query/set/恢复和越界保护；R200 不伪造可调激光功率。
  - `DEPTH_UNITS=0.001m` 保持只读：可写版本会确定性破坏 ROS 顺序启动 IR。
- [ ] 设计 R200 专属 disparity/depth-control 公共 API。
  - RS2 没有这些 RS1 字段的一一对应标准 option；不得借用含义不同的 D400
    option。若扩展公开 enum，需同时给出 ABI、ROS 参数名和最新版迁移方案。
- [x] 在 RS2 SDK 正确支持 Y12I→双路 Y16 IR。
  - 六种尺寸 @30 已验证拆包、metadata 行排除、双目索引、stride、非空像素和
    每次 stop/close；默认 Y8 路径不变。
- [ ] 通过官方格式参数把 Y16 接入 ROS 图像话题。
  - realsense-ros 4.51.1 固定选择 IR Y8；新版官方 wrapper 已有逐流 format
    参数。迁移时使用官方 `infra1_format/infra2_format`，不发明私有参数。
- [ ] 处理混合 FPS 时的 color frame counter scale。
  - RS1 会按 master depth/IR FPS ÷ color FPS 归一化 YUYV 嵌入计数。
  - 当前两个正式预设同频，不受影响；混合 60/30、90/30 前必须补齐。
- [ ] （硬件证据出现前不改）评估可证明的时间戳方案。
  - 当前使用 backend system timestamp 并诚实返回 `SYSTEM_TIME`。
  - RS1 只是按 FPS 合成时间线，不能据此标成 `HARDWARE_CLOCK`。
  - 只有拿到真实设备时钟证据并验证 wrap/drop 后才能更改 domain。
- [ ] 把 stream-intent 从保守 0x7 改为准确 active mask（若固件允许）。
  - 当前完整 close→reopen 已通过；部分 sensor 在线重配仍不保证。
  - 必须验证 depth-only、color-only、IR-only、任意组合和失败回滚。
- [ ] （延期，不阻塞当前发布）热插拔和异常恢复。
  - 覆盖节点重连、XU `ENOENT`、USB reset 后校准重新读取；不刷写固件。
- [ ] 定位 ROS 的 `Frame metadata isn't available` 单次警告。
  - frame counter metadata 已可用；警告来源是 system-time domain。
  - 不允许通过伪造 hardware clock 消除警告。

## P1：D435 通用功能对齐

- [ ] （有设备时验收）连接无 IMU D435 跑相同 ROS2 acceptance。
  - 检查官方默认 profile、图像、CameraInfo、TF、pointcloud 和动态参数。
  - 确认通用 `sensor.cpp` 改动没有破坏 D400/SR300。
- [ ] 增加 capability 示例/文档。
  - 应用按 `supports(option/extension)` 检测 Advanced Mode、硬件同步、IMU、
    emitter，不按设备名猜测。
- [ ] 明确“功能对齐”不包含的硬件能力。
  - R200 不伪装 D400 Advanced Mode、on-chip calibration、硬件同步或 IMU。

## P2：ROS2 质量和性能

- [ ] 分析 1080p30 RGB 对外发布只有约 23-27Hz。
  - 分离 UVC、YUYV→RGB 转换、ROS 序列化、DDS、Python 订阅和 GUI 成本。
  - 比较 pointcloud 开/关、C++ subscriber、composition/intra-process、不同 RMW。
  - 优化不得改变默认 480p60 的稳定性。
- [ ] 修复或删除无输出的 colorizer 路径。
  - 当前 demo 默认关闭 `colorizer.enable`，原始 Z16 和 pointcloud 不受影响。
- [ ] 增加 headless CI/录包回放验收。
  - 无真机测试 profile/filter/calibration；真机 job 单独标记。
- [ ] 校验 QoS、相机命名空间、多相机和 rosbag。

## P2：迁移 librealsense 2.58.3 与新版 ROS2

- [ ] 冻结 2.51.1 已验收 tag，创建独立 migration 分支。
- [ ] 先移植最小核心，不复制旧树中的无关 SR300/ivcam 实现。
- [ ] 解决 product-line bit 冲突。
  - 2.51.1 的 R200 使用 0x20；2.58.3 已把 0x20 分配给 D500。
  - 必须选择新 bit、更新公开枚举/上下文过滤/测试，禁止静默复用。
- [ ] 适配新版 backend、device-info、synthetic sensor、profile/processing、
  metadata 和 matcher 接口。
- [ ] 对齐当时最新版官方 realsense-ros 参数和 launch 风格。
  - R200 差异应限制在设备发现/sensor 建模；上层调用尽量零特例。
- [ ] 完整重跑本文件 P0/P1 验收，并与 2.51.1 结果做回归表。

## 永久约束

- 不刷 R200 固件，不执行破坏性内核/USB 操作。
- 不用假数据宣称 hardware capability。
- 不把构建路径、用户名、序列号或 GitHub 凭据写进代码。
- 不覆盖用户已有改动；提交小而可回退。
- 未跑的硬件验收永远写 `PENDING`，不能凭编译通过改成 `PASS`。
