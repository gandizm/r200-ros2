# 官方基线、改动量与发布许可

## 不是一个仓库，而是两层改动

### librealsense 核心

基线是 IntelRealSense/librealsense 的 `v2.51.1`。当前 `r200-rs2-port`
相对该 tag 的精确差异为：

- 当前 `aff037f` 相对 `v2.51.1` 为 12 个文件、2463 行新增、7 行删除；
- 新增 `src/r200/` 设备、传感器、标定、profile、帧处理和 XU 协议实现；
- 只对 context、产品线、通用 sensor 生命周期和构建入口做少量接线；
- R200 协议和模式以 librealsense 1.12.1（RS1）的 DS4 实现为证据源，转换到
  RS2 的 device/sensor/profile/frame 模型，不把 RS1 整棵代码树复制进来。

这是底层驱动的主要改动，不能描述成“官方原样支持 R200”。准确说法是：
“基于官方 librealsense 2.51.1、参考官方 RS1 R200 实现的社区移植”。

### realsense-ros wrapper

基线是 IntelRealSense/realsense-ros 的 tag `4.51.1`。当前 `r200-ros2`
相对该 tag 的精确差异为：

- 当前 `b02bc1e` 相对 `4.51.1` 为 3 个文件、25 行新增、3 行删除；
- `constants.h` 增加 R200/LR200 PID；
- factory 将两个 PID 交给官方 `BaseRealSenseNode`；
- sensor setup 不再假定视频 profile 必然属于深度/彩色 sensor，以接受 R200
  的独立双红外 sensor。

图像发布、CameraInfo、TF、PointCloud2、QoS、动态参数和 launch 参数仍由官方
wrapper 提供。这里属于可审阅的最小兼容补丁，而不是另写 ROS 驱动。

上述数字按以下命令重算，发布前必须再次记录：

```bash
git diff --shortstat v2.51.1
git diff --shortstat 4.51.1..r200-ros2
```

## Apache-2.0 允许什么

librealsense 和 realsense-ros 的主许可证均为 Apache License 2.0。它允许个人、
研究和商业使用，也允许修改、再发布源码或二进制及销售衍生产品；没有“修改后
必须开源”或“必须把修改贡献回 Intel”的强制要求。本项目仍决定公开发布，便于
复用、审查和后续向上游靠拢。

公开发布时必须做到：

1. 保留并随发行提供 Apache-2.0 `LICENSE`；
2. 修改过的文件应有醒目的修改说明，提交历史和本说明不能替代文件级合规检查；
3. 保留适用的版权、专利、商标和 attribution notice；
4. 保留上游 `NOTICE` 及其中第三方依赖的许可证文本；
5. 不使用 Intel/RealSense 商标暗示官方背书，仓库和 release 明确标为社区移植；
6. 接受 Apache-2.0 的无担保条款；若就该作品发起专利侵权诉讼，相关专利许可
   可能按许可证第 3 节终止。

本说明用于工程合规管理，不替代针对具体商业发行的法律意见。

## 公开仓库结构

- `librealsense`：从官方 `v2.51.1` 公开 fork，开发分支 `r200-rs2-port`；
- `realsense-ros`：从官方 `4.51.1` 公开 fork，开发分支 `r200-ros2`；
- demo、`validation/` 工具、Skill 和中文文档：同一个公开配套仓库，固定上述
  两个 fork 的 commit/tag。

所有 release 都要同时写明官方基线、社区补丁 commit、已验收硬件和未验收项。
