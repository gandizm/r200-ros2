#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Measure the four default R200 ROS image topics and fail on regressions."""

import argparse
import time

import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import Image
from sensor_msgs.msg import PointCloud2


TOPICS = {
    'depth': '/camera/depth/image_rect_raw',
    'color': '/camera/color/image_raw',
    'infra1': '/camera/infra1/image_raw',
    'infra2': '/camera/infra2/image_raw',
}


class ImageRateProbe(Node):
    def __init__(self, require_pointcloud):
        super().__init__('r200_acceptance')
        self.samples = {name: [] for name in TOPICS}
        self.dimensions = {}
        self.pointcloud_samples = []
        self.pointcloud_points = 0
        self._image_subscriptions = []
        for name, topic in TOPICS.items():
            self._image_subscriptions.append(self.create_subscription(
                Image, topic,
                lambda message, stream=name: self.on_image(stream, message),
                qos_profile_sensor_data))
        if require_pointcloud:
            self._pointcloud_subscription = self.create_subscription(
                PointCloud2, '/camera/depth/color/points',
                self.on_pointcloud, qos_profile_sensor_data)

    def on_image(self, stream, message):
        self.samples[stream].append(time.monotonic())
        self.dimensions[stream] = (message.width, message.height)

    def on_pointcloud(self, message):
        self.pointcloud_samples.append(time.monotonic())
        self.pointcloud_points = message.width * message.height


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--duration', type=float, default=6.0)
    parser.add_argument('--width', type=int, default=640)
    parser.add_argument('--height', type=int, default=480)
    parser.add_argument('--color-width', type=int)
    parser.add_argument('--color-height', type=int)
    parser.add_argument('--fps', type=float, default=60.0)
    parser.add_argument('--min-ratio', type=float, default=0.80)
    parser.add_argument('--require-pointcloud', action='store_true')
    parser.add_argument('--pointcloud-min-rate', type=float, default=20.0)
    args = parser.parse_args()

    rclpy.init()
    probe = ImageRateProbe(args.require_pointcloud)
    deadline = time.monotonic() + args.duration
    while rclpy.ok() and time.monotonic() < deadline:
        rclpy.spin_once(probe, timeout_sec=0.1)

    passed = True
    minimum = args.fps * args.min_ratio
    for name in TOPICS:
        samples = probe.samples[name]
        if len(samples) > 1:
            rate = (len(samples) - 1) / (samples[-1] - samples[0])
        else:
            rate = 0.0
        dimensions = probe.dimensions.get(name, (0, 0))
        expected = (args.width, args.height)
        if name == 'color' and args.color_width and args.color_height:
            expected = (args.color_width, args.color_height)
        result = rate >= minimum and dimensions == expected
        passed = passed and result
        print(f'{name}: frames={len(samples)} rate={rate:.2f}Hz '
              f'size={dimensions[0]}x{dimensions[1]} '
              f'result={"PASS" if result else "FAIL"}')

    if args.require_pointcloud:
        samples = probe.pointcloud_samples
        if len(samples) > 1:
            rate = (len(samples) - 1) / (samples[-1] - samples[0])
        else:
            rate = 0.0
        result = rate >= args.pointcloud_min_rate and probe.pointcloud_points > 0
        passed = passed and result
        print(f'pointcloud: frames={len(samples)} rate={rate:.2f}Hz '
              f'points={probe.pointcloud_points} '
              f'result={"PASS" if result else "FAIL"}')

    probe.destroy_node()
    rclpy.shutdown()
    raise SystemExit(0 if passed else 1)


if __name__ == '__main__':
    main()
