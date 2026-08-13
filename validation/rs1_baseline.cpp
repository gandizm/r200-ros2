// SPDX-License-Identifier: Apache-2.0
#include <librealsense/rs.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct FrameSample
{
    bool ok = false;
    unsigned long long number = 0;
    double timestamp_ms = 0.0;
    int width = 0;
    int height = 0;
    int stride = 0;
    int bpp = 0;
    int fps = 0;
    rs::format format = rs::format::any;
    uint64_t checksum = 0;
    size_t bytes = 0;
};

static void add_bytes(uint64_t & h, const unsigned char * p, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
}

int main(int argc, char ** argv)
{
    rs::log_to_console(rs::log_severity::warn);
    try
    {
        rs::context ctx;
        if (ctx.get_device_count() == 0)
        {
            std::cerr << "No RealSense device detected\n";
            return 10;
        }

        rs::device * dev = ctx.get_device(0);
        std::cout << "NAME=" << dev->get_name() << "\n";
        std::cout << "SERIAL=" << dev->get_serial() << "\n";
        std::cout << "FW=" << dev->get_firmware_version() << "\n";
        std::cout << "DEPTH_SCALE=" << std::setprecision(9) << dev->get_depth_scale() << "\n";

        const std::vector<rs::stream> streams = {
            rs::stream::depth, rs::stream::color,
            rs::stream::infrared, rs::stream::infrared2
        };

        // Ensure a clean stream configuration.
        for (auto s : streams)
            dev->disable_stream(s);

        const auto test_one = [&](rs::stream s, int w, int h, rs::format f, int fps)
        {
            for (auto s2 : streams)
                dev->disable_stream(s2);

            dev->enable_stream(s, w, h, f, fps);
            rs::intrinsics intrin = dev->get_stream_intrinsics(s);
            std::cout << "STREAM_TEST " << s << " REQUEST=" << w << "x" << h << "@" << fps << " " << f
                      << " INTRINSIC=" << intrin.width << "x" << intrin.height
                      << " fx=" << intrin.fx << " fy=" << intrin.fy
                      << " ppx=" << intrin.ppx << " ppy=" << intrin.ppy
                      << " model=" << intrin.model()
                      << " coeffs=";
            for (int i = 0; i < 5; ++i)
                std::cout << intrin.coeffs[i] << (i == 4 ? "" : ",");
            std::cout << "\n";

            FrameSample sample;
            std::mutex m;
            dev->set_frame_callback(s, [&](rs::frame f)
            {
                std::lock_guard<std::mutex> lock(m);
                sample.ok = true;
                sample.number = f.get_frame_number();
                sample.timestamp_ms = f.get_timestamp();
                sample.width = f.get_width();
                sample.height = f.get_height();
                sample.stride = f.get_stride();
                sample.bpp = f.get_bpp();
                sample.fps = f.get_framerate();
                sample.format = f.get_format();
                sample.bytes = static_cast<size_t>(sample.stride) * static_cast<size_t>(sample.height);
                sample.checksum = 0;
                if (const void * data = f.get_data())
                    add_bytes(sample.checksum, static_cast<const unsigned char *>(data), sample.bytes);
                if (s == rs::stream::depth && sample.format == rs::format::z16)
                {
                    const uint16_t * d = static_cast<const uint16_t *>(f.get_data());
                    size_t nonzero = 0;
                    uint16_t maxv = 0;
                    size_t n = sample.bytes / 2;
                    for (size_t i = 0; i < n; ++i)
                    {
                        if (d[i]) ++nonzero;
                        if (d[i] > maxv) maxv = d[i];
                    }
                    std::cout << "  DEPTH_STATS nonzero=" << nonzero << " max=" << maxv << "\n";
                }
            });

            dev->start();
            std::this_thread::sleep_for(std::chrono::seconds(2));
            dev->stop();

            std::lock_guard<std::mutex> lock(m);
            std::cout << "  RESULT ok=" << (sample.ok ? 1 : 0)
                      << " actual=" << sample.width << "x" << sample.height
                      << " stride=" << sample.stride << " bpp=" << sample.bpp
                      << " fps=" << sample.fps << " format=" << sample.format
                      << " frame=" << sample.number
                      << " ts_ms=" << std::setprecision(6) << sample.timestamp_ms
                      << " bytes=" << sample.bytes
                      << " checksum=0x" << std::hex << sample.checksum << std::dec
                      << "\n";
        };

        // Single-stream smoke tests.
        test_one(rs::stream::depth, 640, 480, rs::format::z16, 30);
        test_one(rs::stream::color, 640, 480, rs::format::yuyv, 30);
        test_one(rs::stream::infrared, 640, 480, rs::format::y8, 30);
        test_one(rs::stream::infrared2, 640, 480, rs::format::y8, 30);

        // Combined stream test at compatible modes.
        for (auto s : streams)
            dev->disable_stream(s);
        dev->enable_stream(rs::stream::depth, 640, 480, rs::format::z16, 30);
        dev->enable_stream(rs::stream::color, 640, 480, rs::format::yuyv, 30);
        dev->enable_stream(rs::stream::infrared, 640, 480, rs::format::y8, 30);
        dev->enable_stream(rs::stream::infrared2, 640, 480, rs::format::y8, 30);

        struct Pair
        {
            rs::stream from;
            rs::stream to;
        };
        const std::vector<Pair> pairs = {
            {rs::stream::depth, rs::stream::color},
            {rs::stream::depth, rs::stream::infrared},
            {rs::stream::depth, rs::stream::infrared2},
            {rs::stream::infrared, rs::stream::infrared2}
        };
        for (const auto & p : pairs)
        {
            rs::extrinsics e = dev->get_extrinsics(p.from, p.to);
            std::cout << "EXTRINSIC " << p.from << "_TO_" << p.to
                      << " rot=";
            for (int r = 0; r < 9; ++r)
                std::cout << e.rotation[r] << (r == 8 ? "" : ",");
            std::cout << " trans=" << e.translation[0] << "," << e.translation[1] << "," << e.translation[2] << "\n";
        }

        std::vector<FrameSample> samples(streams.size());
        std::mutex m;
        std::atomic<int> remaining(static_cast<int>(streams.size()));
        for (size_t i = 0; i < streams.size(); ++i)
        {
            dev->set_frame_callback(streams[i], [&, i](rs::frame f)
            {
                FrameSample s;
                s.ok = true;
                s.number = f.get_frame_number();
                s.timestamp_ms = f.get_timestamp();
                s.width = f.get_width();
                s.height = f.get_height();
                s.stride = f.get_stride();
                s.bpp = f.get_bpp();
                s.fps = f.get_framerate();
                s.format = f.get_format();
                s.bytes = static_cast<size_t>(s.stride) * static_cast<size_t>(s.height);
                if (const void * data = f.get_data())
                    add_bytes(s.checksum, static_cast<const unsigned char *>(data), s.bytes);
                {
                    std::lock_guard<std::mutex> lock(m);
                    samples[i] = s;
                }
                if (s.ok)
                    --remaining;
            });
        }

        dev->start();
        for (int i = 0; i < 50 && remaining.load() > 0; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        dev->stop();

        for (size_t i = 0; i < streams.size(); ++i)
        {
            const auto & s = samples[i];
            std::cout << "COMBINED " << streams[i]
                      << " ok=" << (s.ok ? 1 : 0)
                      << " actual=" << s.width << "x" << s.height
                      << " stride=" << s.stride << " bpp=" << s.bpp
                      << " fps=" << s.fps << " format=" << s.format
                      << " frame=" << s.number
                      << " ts_ms=" << std::setprecision(6) << s.timestamp_ms
                      << " bytes=" << s.bytes
                      << " checksum=0x" << std::hex << s.checksum << std::dec
                      << "\n";
        }

        return 0;
    }
    catch (const rs::error & e)
    {
        std::cerr << "RS_ERROR failed=" << e.get_failed_function()
                  << " args=" << e.get_failed_args()
                  << " what=" << e.what() << "\n";
        return 2;
    }
    catch (const std::exception & e)
    {
        std::cerr << "STD_ERROR " << e.what() << "\n";
        return 3;
    }
}
