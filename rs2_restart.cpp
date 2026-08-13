// SPDX-License-Identifier: Apache-2.0
#include <librealsense2/rs.hpp>

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

static rs2::stream_profile find_profile(const rs2::sensor & sensor,
                                        rs2_stream stream,
                                        int index)
{
    for (auto && profile : sensor.get_stream_profiles())
    {
        auto video = profile.as<rs2::video_stream_profile>();
        if (profile.stream_type() == stream
            && profile.stream_index() == index
            && profile.format() == (stream == RS2_STREAM_DEPTH
                                     ? RS2_FORMAT_Z16 : RS2_FORMAT_Y8)
            && video.width() == 640 && video.height() == 480
            && video.fps() == 30)
            return profile;
    }
    throw std::runtime_error("required 640x480x30 profile is unavailable");
}

static rs2::stream_profile find_color_profile(const rs2::sensor & sensor)
{
    for (auto && profile : sensor.get_stream_profiles())
    {
        auto video = profile.as<rs2::video_stream_profile>();
        if (profile.stream_type() == RS2_STREAM_COLOR
            && profile.format() == RS2_FORMAT_RGB8
            && video.width() == 640 && video.height() == 480
            && video.fps() == 30)
            return profile;
    }
    throw std::runtime_error("required RGB 640x480x30 profile is unavailable");
}

int main()
{
    try
    {
        rs2::log_to_console(RS2_LOG_SEVERITY_DEBUG);
        rs2::context context;
        auto devices = context.query_devices();
        if (devices.size() == 0)
            throw std::runtime_error("no RealSense device found");

        rs2::sensor depth;
        rs2::sensor infrared;
        rs2::sensor color;
        for (auto && sensor : devices.front().query_sensors())
        {
            const std::string name = sensor.get_info(RS2_CAMERA_INFO_NAME);
            if (name == "Stereo Module") depth = sensor;
            else if (name == "Stereo IR Sensor") infrared = sensor;
            else if (name == "RGB Camera") color = sensor;
        }
        if (!depth || !infrared || !color)
            throw std::runtime_error("R200 depth/IR/color sensors were not all found");

        const auto depth_profile = find_profile(depth, RS2_STREAM_DEPTH, 0);
        const auto ir1_profile = find_profile(infrared, RS2_STREAM_INFRARED, 1);
        const auto ir2_profile = find_profile(infrared, RS2_STREAM_INFRARED, 2);
        const auto color_profile = find_color_profile(color);

        for (int cycle = 1; cycle <= 2; ++cycle)
        {
            std::mutex mutex;
            std::condition_variable ready;
            std::map<rs2_stream, unsigned> counts;
            auto callback = [&](rs2::frame frame)
            {
                std::lock_guard<std::mutex> lock(mutex);
                ++counts[frame.get_profile().stream_type()];
                ready.notify_all();
            };

            depth.open(depth_profile);
            infrared.open({ir1_profile, ir2_profile});
            color.open(color_profile);
            depth.start(callback);
            infrared.start(callback);
            color.start(callback);

            std::unique_lock<std::mutex> lock(mutex);
            const bool received = ready.wait_for(lock, std::chrono::seconds(5), [&]()
            {
                return counts[RS2_STREAM_DEPTH] >= 2
                    && counts[RS2_STREAM_INFRARED] >= 4
                    && counts[RS2_STREAM_COLOR] >= 2;
            });
            lock.unlock();

            color.stop();
            infrared.stop();
            depth.stop();
            color.close();
            infrared.close();
            depth.close();

            std::cout << "CYCLE=" << cycle
                      << " depth=" << counts[RS2_STREAM_DEPTH]
                      << " infrared=" << counts[RS2_STREAM_INFRARED]
                      << " color=" << counts[RS2_STREAM_COLOR]
                      << " result=" << (received ? "PASS" : "FAIL") << "\n";
            if (!received)
                return 4;
        }
        return 0;
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
