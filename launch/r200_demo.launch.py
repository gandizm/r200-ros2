#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Launch the Intel R200 through the official realsense2_camera node plus RViz2.
#
# Usage:
#   ros2 launch r200_demo r200_demo.launch.py gui:=false
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    depth_profile = LaunchConfiguration('depth_profile')
    ir_profile = LaunchConfiguration('ir_profile')
    color_profile = LaunchConfiguration('color_profile')

    # Use the official package/executable and official ROS parameter names.
    # Stereo IR Sensor is a separate R200 UVC sensor, so its generated profile
    # parameter must be set explicitly; rs_launch.py does not declare it.
    camera = Node(
        package='realsense2_camera',
        executable='realsense2_camera_node',
        namespace='camera',
        name='camera',
        parameters=[{
            'enable_depth': True,
            'enable_color': True,
            'enable_infra1': True,
            'enable_infra2': True,
            'depth_module.profile': depth_profile,
            'stereo_ir_sensor.profile': ir_profile,
            'rgb_camera.profile': color_profile,
            'enable_sync': True,
            'pointcloud.enable': LaunchConfiguration('pointcloud'),
            'align_depth.enable': False,
            'colorizer.enable': False,
        }],
        output='screen',
        arguments=['--ros-args', '--log-level', LaunchConfiguration('log_level')],
        emulate_tty=True,
    )

    rviz_config = os.path.join(get_package_share_directory('r200_demo'),
                               'config', 'r200.rviz')
    rviz = Node(
        condition=IfCondition(LaunchConfiguration('gui')),
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config],
        output='screen',
    )

    # Independent, resizable image windows (one rqt_image_view per stream).
    image_viewers = []
    for stream, topic in (
        ('depth', '/camera/depth/image_rect_raw'),
        ('color', '/camera/color/image_raw'),
        ('infra1', '/camera/infra1/image_raw'),
        ('infra2', '/camera/infra2/image_raw'),
    ):
        image_viewers.append(Node(
            condition=IfCondition(LaunchConfiguration('gui')),
            package='rqt_image_view',
            executable='rqt_image_view',
            name='image_view_' + stream,
            arguments=[topic],
            output='screen',
        ))

    return LaunchDescription([
        DeclareLaunchArgument('depth_profile', default_value='640x480x60'),
        DeclareLaunchArgument('ir_profile', default_value='640x480x60'),
        DeclareLaunchArgument('color_profile', default_value='640x480x60'),
        DeclareLaunchArgument('pointcloud', default_value='true'),
        DeclareLaunchArgument('gui', default_value='true'),
        DeclareLaunchArgument('log_level', default_value='info'),
        camera,
        rviz,
        *image_viewers,
    ])
