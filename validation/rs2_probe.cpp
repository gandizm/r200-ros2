// SPDX-License-Identifier: Apache-2.0
#include <librealsense2/rs.hpp>

#include <iomanip>
#include <iostream>
#include <string>

int main()
{
    try
    {
        rs2::context ctx;
        auto devices = ctx.query_devices();
        std::cout << "DEVICE_COUNT=" << devices.size() << "\n";

        for (auto && dev : devices)
        {
            std::cout << "DEVICE name=" << dev.get_info(RS2_CAMERA_INFO_NAME)
                      << " serial=" << dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)
                      << " fw=" << dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION)
                      << " product_line=" << dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE)
                      << "\n";

            std::cout << "SENSOR_COUNT=" << dev.query_sensors().size() << "\n";
            for (auto && sensor : dev.query_sensors())
            {
                std::cout << "  SENSOR name=" << sensor.get_info(RS2_CAMERA_INFO_NAME) << "\n";
                auto profiles = sensor.get_stream_profiles();
                for (auto && p : profiles)
                {
                    auto vp = p.as<rs2::video_stream_profile>();
                    std::cout << "    PROFILE " << p.stream_name()
                              << " index=" << p.stream_index()
                              << " " << vp.width() << "x" << vp.height()
                              << "@" << vp.fps() << " " << p.format()
                              << " uid=" << p.unique_id() << "\n";
                }
            }
        }

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
