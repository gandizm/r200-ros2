// SPDX-License-Identifier: Apache-2.0
#include <librealsense2/rs.hpp>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <utility>
#include <vector>

struct result
{
    int count[2] = {0, 0};
    bool valid = true;
    uint16_t max_value[2] = {0, 0};
    size_t nonzero[2] = {0, 0};
};

int main()
{
    try
    {
        rs2::context ctx;
        auto devices = ctx.query_devices();
        if (devices.size() == 0)
            throw std::runtime_error("no RealSense device");

        rs2::sensor ir;
        for (auto && sensor : devices.front().query_sensors())
            if (std::string(sensor.get_info(RS2_CAMERA_INFO_NAME)) == "Stereo IR Sensor")
                ir = sensor;
        if (!ir)
            throw std::runtime_error("R200 Stereo IR Sensor not found");

        const std::vector<std::pair<int, int>> sizes = {
            {640, 480}, {628, 468}, {492, 372},
            {480, 360}, {332, 252}, {320, 240}
        };
        int failures = 0;
        for (const auto & size : sizes)
        {
            rs2::stream_profile selected[2];
            for (auto && profile : ir.get_stream_profiles())
            {
                auto video = profile.as<rs2::video_stream_profile>();
                if (profile.stream_type() == RS2_STREAM_INFRARED
                    && profile.format() == RS2_FORMAT_Y16
                    && profile.fps() == 30
                    && video.width() == size.first && video.height() == size.second
                    && (profile.stream_index() == 1 || profile.stream_index() == 2))
                    selected[profile.stream_index() - 1] = profile;
            }
            if (!selected[0] || !selected[1])
                throw std::runtime_error("missing Y16 profile");

            result state;
            std::mutex mutex;
            std::condition_variable ready;
            ir.open({selected[0], selected[1]});
            ir.start([&](rs2::frame frame)
            {
                auto video = frame.as<rs2::video_frame>();
                const int side = frame.get_profile().stream_index() - 1;
                if (!video || side < 0 || side > 1)
                    return;

                std::lock_guard<std::mutex> lock(mutex);
                state.valid = state.valid
                    && video.get_width() == size.first
                    && video.get_height() == size.second
                    && video.get_bytes_per_pixel() == 2
                    && video.get_stride_in_bytes() == size.first * 2
                    && static_cast<size_t>(video.get_data_size())
                        == static_cast<size_t>(size.first) * size.second * 2;
                const auto * pixels = reinterpret_cast<const uint16_t *>(video.get_data());
                const size_t count = static_cast<size_t>(size.first) * size.second;
                for (size_t i = 0; i < count; ++i)
                {
                    state.max_value[side] = std::max(state.max_value[side], pixels[i]);
                    state.nonzero[side] += pixels[i] != 0;
                }
                ++state.count[side];
                ready.notify_all();
            });

            {
                std::unique_lock<std::mutex> lock(mutex);
                ready.wait_for(lock, std::chrono::seconds(3), [&]()
                {
                    return state.count[0] >= 3 && state.count[1] >= 3;
                });
            }
            ir.stop();
            ir.close();

            const bool pass = state.valid && state.count[0] >= 3 && state.count[1] >= 3
                && state.nonzero[0] > 0 && state.nonzero[1] > 0
                && state.max_value[0] > 0 && state.max_value[1] > 0;
            std::cout << "Y16 " << size.first << "x" << size.second << "@30"
                      << " frames=" << state.count[0] << "/" << state.count[1]
                      << " nonzero=" << state.nonzero[0] << "/" << state.nonzero[1]
                      << " max=" << state.max_value[0] << "/" << state.max_value[1]
                      << " result=" << (pass ? "PASS" : "FAIL") << "\n";
            failures += !pass;
        }

        std::cout << "Y16_MATRIX total=" << sizes.size()
                  << " failures=" << failures
                  << " result=" << (failures ? "FAIL" : "PASS") << "\n";
        return failures ? 1 : 0;
    }
    catch (const std::exception & error)
    {
        std::cerr << "ERROR " << error.what() << "\n";
        return 2;
    }
}
