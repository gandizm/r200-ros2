# R200 后续 TODO（交给接管 Codex）

按优先级排序。每一项都说明现状与建议做法，避免走弯路。

## P0 — 运行中切换 IR 失败（firmware 限制）

现状：realsense2_camera 通过参数重启 sensor 时，IR 接口的
`VIDIOC_S_FMT` 返回 `EIO`，只有重启整个节点/重插 USB 才恢复。
深度、颜色传感器可运行中切换，IR 不行；因此 demo 的切换脚本采用
“重启 launch”。

方向（按可能性排序）：
1. 对照 RS1 `ds_device::on_before_start()`：每次 STREAMON 前重写
   `stream_intent=0x7`（现在 RS2 只在每个设备实例首次 open 时写一次）。
   `src/r200/r200.cpp` 的 `write_stream_intent()` 目前有 `_intent_written`
   一次性保护；若在全部流停止后复位该标志（跟踪所有 sensor 的 stop），
   重启时的第一个 open 会重写 intent。之前试过“每次 open 都写”方案：
   首启没问题，但重启仍 EIO（怀疑需要整机 stop→intent→start 的原子序列）。
2. 研究 RS1 的 `sw_reset`/`disparity` XU 是否需要在重配置前下发。
3. 若确实无法运行中切换：把切换设计为“进程级重启”并保持现状，
   在文档中标注为已知限制（当前做法）。

## P0 — 时间戳/元数据（`Frame metadata isn't available`）

现状：R200 timestamp 域为 `RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME`，驱动每帧打
警告；RGB 点云纹理在高帧率下偶发时间戳对齐失败。

方向：移植 RS1 的帧计数解析（`src/ds-device.cpp` 里
`serial_timestamp_generator` / fisheye timestamp reader）：R200 把帧计数
嵌在 IR 首像素低 4 位（fw < 1.27.2.90）或首 4 像素 LSB（fw >= 1.27.2.90，
本机 fw 2.0.71.14 走后者）。实现一个 `r200_timestamp_reader`，在
`src/r200/r200.cpp` 的 `r200_timestamp_reader::get_frame_timestamp` 中解析，
并让 `get_frame_timestamp_domain` 返回 `HARDWARE_CLOCK`。需要同步 IR 与
depth 的计数（当前二者独立走各自 raw sensor）。

## P1 — 原生分辨率 vs 640x480 的策略（已基本解决）

现状（本轮已实现）：depth 同时暴露
- 原生 `628x469 / 628x361 / 628x242`（零处理，identity block）；
- rectified `640x480 / 492x372 / 332x252`（`r200_depth_pad`，6px 零填充，
  非插值）。

两者内参同源（`modesLR[]`）：原生 `ppx/ppy = rectified ppx/ppy - 6`，
`fx/fy` 相同，3D 几何一致。彩色 UVC 最高 1920x1080@30，无 480p 限制。
ROS2 建图算法（RTAB-Map 等）用 camera_info，不要求 480p 或 8/16 对齐；
唯一的多重限制（RS2 内 `(w*h)%8==0` 断言）已在 `src/sensor.cpp` 放宽。

建议：保留双 profile。后续可选：给 demo 增加 `--raw` 一键切换；把默认
profile 的选择写成 launch 参数文档。

## P1 — `/camera/depth/colorized` 无输出

现状：`colorizer.enable:=true` 后话题不发布（echo 无消息）。原因未定位，
可能与 align_depth 关闭时 colorizer 的 frame 路径有关。

方向：读 `base_realsense_node.cpp` 的 `frame_callback` 中
`_colorizer_filter->Process` 分支，确认是否需要 `align_depth.enable:=true`
或帧集合约束。demo 目前用原始 Z16（RViz 可 Normalize 显示），不依赖此话题。

## P1 — 60Hz 全链路只有 ~41Hz

现状：RS2 直连 depth-only 640x480@60 ≈ 56.6Hz；ROS2 全链路（depth60 +
color60 + IR30 + pointcloud）≈ 41Hz。

方向：先关 pointcloud（`pointcloud.enable:=false`）测纯图像路径，再逐步
加回 color/IR，定位是 pointcloud 计算、asyncer 单线程还是发布开销。
注意 realsense2_camera 默认 `use_intra_process_comms=true`，可对比关闭后的
吞吐。

## P2 — 向新版 librealsense2 迁移（用户后续主线）

1. 当前基线是 2.51.1（官方最后一个保留 SR300 legacy 的版本），本分支
   `r200-rs2-port` 相对上游的最小补丁集 = `git diff 上游2.51.1..HEAD`，
   集中在 `src/r200/`、`src/context.cpp`、`src/sensor.cpp`、
   `include/librealsense2/h/rs_context.h`、`src/libusb/libusb.h`。
2. 新版 RS2 删除了 `src/ivcam` 等 legacy 代码，但 R200 用的是通用
   `synthetic_sensor` + `uvc_sensor` + processing block，理论上可随
   `src/r200/` 平移。需要适配：新 `backend` 接口、`stream_profile`/
   `stream_resolution` 机制是否变化、metadata 框架。
3. 先在新版上跑 `rs2_probe`/`rs2_stream`（tools/），再跑 ROS2 驱动。

## P2 — 长稳测试与边界

- 连续 >1h 四路流 + pointcloud 稳定性（当前只跑过分钟级）。
- 628x361 / 628x242 与 492x372 / 332x252 的端到端验证（profile 已暴露）。
- 热插拔恢复、`usbreset` 后的 XU `ENOENT` 恢复路径（见 HANDOFF.md 第 6 节）。

## 约束（务必遵守）

- 不刷写 R200 固件；不做 destructive kernel 操作。
- 不改 D400/SR300/common 代码，除非是通用 bug（历史上有两处通用修复：
  `sensor.cpp` 的 backend 尺寸/断言，需保留并说明）。
- 每个小改动单独 commit（`r200: ...` / `ros2: ...`），保持可回退。
