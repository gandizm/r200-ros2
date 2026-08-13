// SPDX-License-Identifier: Apache-2.0
#include <librealsense2/rs.hpp>

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

static rs2::stream_profile find_profile(const rs2::sensor & sensor,
                                        rs2_stream stream,
                                        int index,
                                        int width,
                                        int height,
                                        int fps)
{
    const rs2_format format = stream == RS2_STREAM_DEPTH
        ? RS2_FORMAT_Z16 : RS2_FORMAT_Y8;
    for (auto && profile : sensor.get_stream_profiles())
    {
        auto video = profile.as<rs2::video_stream_profile>();
        if (profile.stream_type() == stream
            && profile.stream_index() == index
            && profile.format() == format
            && video.width() == width && video.height() == height
            && video.fps() == fps)
            return profile;
    }
    throw std::runtime_error("required profile is unavailable");
}

int main()
{
    try
    {
        rs2::context context;
        auto devices = context.query_devices();
        if (devices.size() == 0)
            throw std::runtime_error("no RealSense device found");

        rs2::sensor depth;
        rs2::sensor infrared;
        for (auto && sensor : devices.front().query_sensors())
        {
            const std::string name = sensor.get_info(RS2_CAMERA_INFO_NAME);
            if (name == "Stereo Module") depth = sensor;
            else if (name == "Stereo IR Sensor") infrared = sensor;
        }
        if (!depth || !infrared)
            throw std::runtime_error("R200 depth and IR sensors were not found");

        const std::vector<std::pair<int, int>> sizes = {
            {640, 480}, {628, 468}, {492, 372},
            {480, 360}, {332, 252}, {320, 240}
        };
        const std::vector<int> rates = {30, 60, 90};
        unsigned failures = 0;

        for (const auto & size : sizes)
        {
            for (const int fps : rates)
            {
                const auto dp = find_profile(depth, RS2_STREAM_DEPTH, 0,
                                             size.first, size.second, fps);
                const auto ip1 = find_profile(infrared, RS2_STREAM_INFRARED, 1,
                                              size.first, size.second, fps);
                const auto ip2 = find_profile(infrared, RS2_STREAM_INFRARED, 2,
                                              size.first, size.second, fps);

                std::mutex mutex;
                std::condition_variable ready;
                unsigned depth_count = 0;
                unsigned ir_count = 0;
                bool valid = true;
                auto callback = [&](rs2::frame frame)
                {
                    auto video = frame.as<rs2::video_frame>();
                    std::lock_guard<std::mutex> lock(mutex);
                    const bool has_counter = frame.supports_frame_metadata(
                        RS2_FRAME_METADATA_FRAME_COUNTER);
                    const auto counter = has_counter
                        ? frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER) : 0;
                    valid = valid
                        && video.get_width() == size.first
                        && video.get_height() == size.second
                        && video.get_stride_in_bytes() >= size.first
                            * video.get_bytes_per_pixel()
                        && frame.get_data() != nullptr
                        && frame.get_data_size() >= static_cast<size_t>(
                            video.get_stride_in_bytes() * size.second)
                        && has_counter
                        && counter > 0
                        && static_cast<unsigned long long>(counter)
                            == frame.get_frame_number();
                    if (frame.get_profile().stream_type() == RS2_STREAM_DEPTH)
                        ++depth_count;
                    else if (frame.get_profile().stream_type() == RS2_STREAM_INFRARED)
                        ++ir_count;
                    ready.notify_all();
                };

                depth.open(dp);
                infrared.open({ip1, ip2});
                depth.start(callback);
                infrared.start(callback);

                std::unique_lock<std::mutex> lock(mutex);
                const bool received = ready.wait_for(
                    lock, std::chrono::seconds(5),
                    [&]() { return depth_count >= 2 && ir_count >= 4; });
                lock.unlock();

                infrared.stop();
                depth.stop();
                infrared.close();
                depth.close();

                const bool passed = received && valid;
                failures += passed ? 0 : 1;
                std::cout << size.first << "x" << size.second << "@" << fps
                          << " depth=" << depth_count
                          << " infrared=" << ir_count
                          << " result=" << (passed ? "PASS" : "FAIL") << "\n";
            }
        }

        std::cout << "MATRIX total=" << sizes.size() * rates.size()
                  << " failures=" << failures
                  << " result=" << (failures == 0 ? "PASS" : "FAIL") << "\n";
        return failures == 0 ? 0 : 4;
    }
    catch (const rs2::error & error)
    {
        std::cerr << "RS2_ERROR " << error.what() << "\n";
        return 2;
    }
    catch (const std::exception & error)
    {
        std::cerr << "ERROR " << error.what() << "\n";
        return 3;
    }
}
