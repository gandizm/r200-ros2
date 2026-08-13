// SPDX-License-Identifier: Apache-2.0
// Minimal RS1 probe: enumerate devices, stream modes, intrinsics and log errors.
#include <librealsense/rs.hpp>
#include <iostream>

int main(int argc, char** argv)
{
    rs::log_to_console(rs::log_severity::debug);
    try
    {
        rs::context ctx;
        int count = ctx.get_device_count();
        std::cout << "DEVICE_COUNT=" << count << std::endl;
        for (int i = 0; i < count; ++i)
        {
            rs::device* dev = ctx.get_device(i);
            std::cout << "DEVICE[" << i << "] name=" << dev->get_name()
                      << " serial=" << dev->get_serial()
                      << " fw=" << dev->get_firmware_version() << std::endl;
            for (int s = 0; s < RS_STREAM_COUNT; ++s)
            {
                rs::stream strm = (rs::stream)s;
                int modes = dev->get_stream_mode_count(strm);
                if (!modes) continue;
                std::cout << "  STREAM " << strm << " modes=" << modes << std::endl;
                for (int k = 0; k < modes; ++k)
                {
                    int w=0,h=0,fps=0; rs::format fmt=(rs::format)RS_FORMAT_ANY;
                    dev->get_stream_mode(strm,k,w,h,fmt,fps);
                    std::cout << "    " << w << "x" << h << "@" << fps << " " << fmt << std::endl;
                }
            }
        }
    }
    catch (const rs::error& e)
    {
        std::cerr << "RS_ERROR failed=" << e.get_failed_function()
                  << " args=" << e.get_failed_args()
                  << " what=" << e.what() << std::endl;
        return 2;
    }
    catch (const std::exception& e)
    {
        std::cerr << "STD_ERROR " << e.what() << std::endl;
        return 3;
    }
    return 0;
}
