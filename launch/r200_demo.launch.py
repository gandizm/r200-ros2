#!/usr/bin/env python3
# Launch the Intel R200 through the official realsense2_camera driver plus RViz2.
#
# Usage:
#   ros2 launch r200_demo r200_demo.launch.py depth_profile:=640x480x60
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    depth_profile = LaunchConfiguration(
        'depth_profile', default='640x480x60')
    color_profile = LaunchConfiguration(
        'color_profile', default='640x480x60')

    rs_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory('realsense2_camera'),
                'launch', 'rs_launch.py')),
        launch_arguments={
            'pointcloud.enable': 'true',
            'colorizer.enable': 'true',
            'align_depth.enable': 'false',
            'enable_infra1': 'true',
            'enable_infra2': 'true',
            'depth_module.profile': depth_profile,
            'rgb_camera.profile': color_profile,
        }.items(),
    )

    rviz_config = os.path.join(
        os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
        'config', 'r200.rviz')
    rviz = Node(
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
            package='rqt_image_view',
            executable='rqt_image_view',
            name='image_view_' + stream,
            arguments=[topic],
            output='screen',
        ))

    return LaunchDescription([
        DeclareLaunchArgument('depth_profile', default_value='640x480x60'),
        DeclareLaunchArgument('color_profile', default_value='640x480x60'),
        rs_launch,
        rviz,
        *image_viewers,
    ])
