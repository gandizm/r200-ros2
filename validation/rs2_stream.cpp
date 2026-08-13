// SPDX-License-Identifier: Apache-2.0
#include <librealsense2/rs.hpp>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>

static uint64_t checksum(const void * data, size_t bytes)
{
    const unsigned char * p = static_cast<const unsigned char *>(data);
    uint64_t h = 0;
    for (size_t i = 0; i < bytes; ++i)
    {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static void print_intrinsics(const char * label, const rs2_intrinsics & i)
{
    std::cout << label
              << " " << i.width << "x" << i.height
              << " fx=" << i.fx << " fy=" << i.fy
              << " ppx=" << i.ppx << " ppy=" << i.ppy
              << " model=" << i.model
              << " coeffs=" << i.coeffs[0] << "," << i.coeffs[1] << ","
              << i.coeffs[2] << "," << i.coeffs[3] << "," << i.coeffs[4] << "\n";
}

static void print_frame_clock(const char * label, const rs2::frame & frame)
{
    if (!frame)
        return;
    std::cout << "  " << label
              << " frame=" << frame.get_frame_number()
              << " domain=" << frame.get_frame_timestamp_domain();
    if (frame.supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
        std::cout << " embedded_counter="
                  << frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
    else
        std::cout << " embedded_counter=unavailable";
    std::cout << "\n";
}

int main(int argc, char ** argv)
{
    try
    {
        rs2::log_to_console(RS2_LOG_SEVERITY_DEBUG);
        rs2::context ctx;
        auto devices = ctx.query_devices();
        if (devices.size() == 0)
        {
            std::cerr << "No device\n";
            return 10;
        }
        for (auto && dev : devices)
        {
            std::cout << "PRE_START name=" << dev.get_info(RS2_CAMERA_INFO_NAME)
                      << " serial=" << dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)
                      << " fw=" << dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION) << "\n";
        }

        std::string mode = (argc > 1) ? argv[1] : "all";
        rs2::pipeline pipe(ctx);
        rs2::config cfg;
        if (mode == "depth")
            cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
        else if (mode == "raw")
        {
            cfg.enable_stream(RS2_STREAM_DEPTH, 628, 468, RS2_FORMAT_Z16, 30);
            cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_RGB8, 30);
        }
        else if (mode == "ir")
            cfg.enable_stream(RS2_STREAM_INFRARED, 1, 640, 480, RS2_FORMAT_Y8, 30);
        else if (mode == "color")
            cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_RGB8, 30);
        else
        {
            cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
            cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_RGB8, 30);
            cfg.enable_stream(RS2_STREAM_INFRARED, 1, 640, 480, RS2_FORMAT_Y8, 30);
            cfg.enable_stream(RS2_STREAM_INFRARED, 2, 640, 480, RS2_FORMAT_Y8, 30);
        }

        auto profile = pipe.start(cfg);
        auto dev = profile.get_device();
        std::cout << "DEVICE name=" << dev.get_info(RS2_CAMERA_INFO_NAME)
                  << " serial=" << dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)
                  << " fw=" << dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION) << "\n";

        for (int n = 0; n < 30; ++n)
        {
            auto frames = pipe.wait_for_frames();
            auto depth = frames.get_depth_frame();
            auto color = frames.get_color_frame();
            rs2::frame ir1_f, ir2_f;
            for (auto && f : frames)
            {
                if (f.get_profile().stream_type() == RS2_STREAM_INFRARED &&
                    f.get_profile().format() == RS2_FORMAT_Y8)
                {
                    if (f.get_profile().stream_index() == 1)
                        ir1_f = f;
                    else if (f.get_profile().stream_index() == 2)
                        ir2_f = f;
                }
            }
            auto vir1 = ir1_f.as<rs2::video_frame>();
            auto vir2 = ir2_f.as<rs2::video_frame>();

            if (mode == "raw")
            {
                if (!depth || !color)
                    continue;
            }
            else if (!depth || !color || !vir1 || !vir2)
                continue;

            std::cout << "FRAME_SET " << n
                      << " ts=" << frames.get_timestamp()
                      << " frame=" << frames.get_frame_number()
                      << " depth=" << depth.get_width() << "x" << depth.get_height()
                      << " depth_units=" << depth.get_units()
                      << " color=" << color.get_width() << "x" << color.get_height();
            if (vir1 && vir2)
                std::cout << " ir1=" << vir1.get_width() << "x" << vir1.get_height()
                          << " ir2=" << vir2.get_width() << "x" << vir2.get_height();
            std::cout << "\n";
            print_frame_clock("DEPTH_CLOCK", depth);
            print_frame_clock("COLOR_CLOCK", color);
            print_frame_clock("IR1_CLOCK", ir1_f);
            print_frame_clock("IR2_CLOCK", ir2_f);

            auto dprofile = depth.get_profile().as<rs2::video_stream_profile>();
            auto cprofile = color.get_profile().as<rs2::video_stream_profile>();
            print_intrinsics("  DEPTH_INTRINSICS", dprofile.get_intrinsics());
            print_intrinsics("  COLOR_INTRINSICS", cprofile.get_intrinsics());

            auto ext = dprofile.get_extrinsics_to(cprofile);
            std::cout << "  EXTRINSIC depth_to_color rot="
                      << ext.rotation[0] << "," << ext.rotation[1] << "," << ext.rotation[2]
                      << "," << ext.rotation[3] << "," << ext.rotation[4] << "," << ext.rotation[5]
                      << "," << ext.rotation[6] << "," << ext.rotation[7] << "," << ext.rotation[8]
                      << " trans=" << ext.translation[0] << "," << ext.translation[1] << ","
                      << ext.translation[2] << "\n";

            auto dsize = depth.get_stride_in_bytes() * depth.get_height();
            auto csize = color.get_stride_in_bytes() * color.get_height();
            std::cout << "  CHECKSUMS depth=0x" << std::hex << checksum(depth.get_data(), dsize)
                      << " color=0x" << checksum(color.get_data(), csize);
            if (vir1 && vir2)
            {
                auto isize = vir1.get_stride_in_bytes() * vir1.get_height();
                std::cout << " ir1=0x" << checksum(vir1.get_data(), isize)
                          << " ir2=0x" << checksum(vir2.get_data(), isize);
            }
            std::cout << std::dec << "\n";
            if (depth.get_width() == 640 && depth.get_height() == 480)
            {
                const uint16_t * d = reinterpret_cast<const uint16_t *>(depth.get_data());
                const int dw = depth.get_width();
                std::cout << "  DEPTH_SAMPLES border=" << d[0] << "," << d[5 * dw + 5]
                          << " inner=" << d[6 * dw + 6] << "," << d[240 * dw + 320]
                          << " far=" << d[473 * dw + 633] << "," << d[479 * dw + 639] << "\n";
                size_t nonzero = 0;
                uint16_t maxv = 0;
                for (size_t i = 0; i < static_cast<size_t>(dw) * depth.get_height(); ++i)
                {
                    if (d[i]) ++nonzero;
                    if (d[i] > maxv) maxv = d[i];
                }
                std::cout << "  DEPTH_STATS nonzero=" << nonzero << " max=" << maxv << "\n";
            }
            break;
        }

        pipe.stop();
        return 0;
    }
    catch (const rs2::error & e)
    {
        std::cerr << "RS2_ERROR failed=" << e.get_failed_function()
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
