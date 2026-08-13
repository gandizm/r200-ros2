# R200 ROS2 demo

最接近英特尔官方 RS2+ROS2 用法的最小演示：官方 `realsense2_camera`
驱动 + RViz2（点云）+ 4 个独立的 `rqt_image_view` 图像窗口
（Depth / RGB / Infra1 / Infra2），支持切换分辨率与帧率。

## 环境准备

本机已经安装：ROS2 Humble、RViz2、本工作区的 `realsense2_camera`
（链接 R200 适配版 librealsense2，安装前缀 `rs2_install`）。

```bash
source /opt/ros/humble/setup.bash
source /home/zmiaow/r200_ros2/ros2_ws/install/setup.bash
export LD_LIBRARY_PATH=/home/zmiaow/r200_ros2/rs2_install/lib:$LD_LIBRARY_PATH
```

## 一键启动

```bash
ros2 launch r200_demo r200_demo.launch.py depth_profile:=640x480x60
```

RViz2 显示 `/camera/depth/color/points` 的 RGB 点云（固定坐标系
`camera_link`）；Depth / RGB / Infra1 / Infra2 各自独立成窗，可自由
拖拽、缩放。

## 运行中切换分辨率

```bash
# 切到 30Hz
ros2 run r200_demo r200_switch.sh 640x480x30 640x480x30

# 深度 90Hz + 颜色 720p
ros2 run r200_demo r200_switch.sh 640x480x90 1280x720x30

# 默认 640x480x60
ros2 run r200_demo r200_switch.sh

# 原生深度 628x469（无任何 padding/重采样）+ 颜色 640x480
ros2 run r200_demo r200_switch.sh 628x469x60 640x480x60
```

可用组合：

| Stream | 可用分辨率/帧率（节选） |
| --- | --- |
| Depth | 原生 628x469@30/60/90、628x361、628x242；rectified 640x480@30/60/90、492x372、332x252 |
| Infra1/2 | 同 Depth（Y8） |
| Color | 1920x1080@30, 1280x720@30, 640x480@60, 320x240@60 等 |

关于“重采样”：

- 640x480 是 **6px 零填充**（原生像素 1:1 平移到 offset(6,6)，边框置 0），
  不是插值重采样，像素值原样保留。
- 原生 628x469 的内参 `fx=573.909, ppx=314.807, ppy=233.195` 与 640x480 的
  `fx=573.909, ppx=320.807, ppy=239.195` 来自同一份标定，两者 3D 点云几何
  **完全一致**；depth→color 外参不变。

说明：R200 固件不允许在其他流运行时重配置 IR 接口，因此切换采用
“停掉当前 launch → 用新参数重启”的方式，RViz 会一并重开。
实测 640x480 深度 60Hz 可用（`ros2 topic hz /camera/depth/image_rect_raw`）。

## 常用话题

- `/camera/depth/image_rect_raw` + `/camera/depth/camera_info`
- `/camera/color/image_raw` + `/camera/color/camera_info`
- `/camera/infra1/image_raw`、`/camera/infra2/image_raw`
- `/camera/depth/color/points`（PointCloud2）
- `/camera/depth/colorized`（深度伪彩色）
- `/tf_static`（`camera_link` 到各光学坐标系）
