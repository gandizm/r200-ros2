// SPDX-License-Identifier: Apache-2.0
#include <librealsense2/rs.hpp>

#include <cmath>
#include <iostream>

int main()
{
    try
    {
        rs2::pipeline pipe;
        rs2::config cfg;
        cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
        cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_RGB8, 30);

        auto profile = pipe.start(cfg);
        auto dev = profile.get_device();
        std::cout << "DEVICE name=" << dev.get_info(RS2_CAMERA_INFO_NAME)
                  << " serial=" << dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) << "\n";

        rs2::pointcloud pc;

        auto frames = pipe.wait_for_frames();
        auto depth = frames.get_depth_frame();
        auto color = frames.get_color_frame();
        if (!depth)
        {
            std::cerr << "No depth frame\n";
            return 2;
        }
        if (color)
            pc.map_to(color);

        auto points = pc.calculate(depth);
        auto vertices = points.get_vertices();
        size_t valid = 0;
        double sum = 0;
        for (size_t i = 0; i < points.size(); ++i)
        {
            const rs2::vertex & v = vertices[i];
            if (std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z))
            {
                ++valid;
                sum += v.z;
            }
        }
        std::cout << "POINTS size=" << points.size() << " valid=" << valid
                  << " mean_z=" << (valid ? sum / valid : 0.0) << "\n";

        const int dw = depth.get_width();
        const rs2::vertex center = vertices[240 * dw + 320];
        std::cout << "VERTEX center(" << center.x << ", " << center.y << ", " << center.z
                  << ") textured=" << points.get_texture_coordinates() << "\n";

        pipe.stop();
        return 0;
    }
    catch (const rs2::error & e)
    {
        std::cerr << "RS2_ERROR failed=" << e.get_failed_function()
                  << " args=" << e.get_failed_args() << " what=" << e.what() << "\n";
        return 2;
    }
    catch (const std::exception & e)
    {
        std::cerr << "STD_ERROR " << e.what() << "\n";
        return 3;
    }
}
