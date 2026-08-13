// SPDX-License-Identifier: Apache-2.0
#include <librealsense2/rs.hpp>

#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    try
    {
        rs2::pipeline pipe;
        rs2::config cfg;
        cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 60);
        auto profile = pipe.start(cfg);
        auto sensor = profile.get_device().first<rs2::depth_sensor>();
        (void)sensor;

        int count = 0;
        auto t0 = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - t0 < std::chrono::seconds(5))
        {
            auto frames = pipe.wait_for_frames(5000);
            if (frames.get_depth_frame())
                ++count;
        }
        double secs = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - t0).count();
        std::cout << "depth 640x480@60 requested: " << count << " frames in "
                  << secs << "s = " << count / secs << " Hz\n";
        pipe.stop();
        return 0;
    }
    catch (const rs2::error & e)
    {
        std::cerr << "RS2_ERROR " << e.what() << "\n";
        return 2;
    }
}
