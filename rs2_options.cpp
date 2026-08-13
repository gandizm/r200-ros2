// SPDX-License-Identifier: Apache-2.0
#include <librealsense2/rs.hpp>

#include <iomanip>
#include <iostream>

int main(int argc, char ** argv)
{
    try
    {
        rs2::context ctx;
        auto devices = ctx.query_devices();
        if (devices.size() == 0)
        {
            std::cerr << "NO_DEVICE\n";
            return 1;
        }

        const std::string mode = argc == 2 ? argv[1] : "";
        const bool write_depth_units = mode == "--write-depth-units";
        const bool test_color_auto = mode == "--test-color-auto";
        for (auto && sensor : devices.front().query_sensors())
        {
            std::cout << "SENSOR " << sensor.get_info(RS2_CAMERA_INFO_NAME) << "\n";
            for (int value = 0; value < RS2_OPTION_COUNT; ++value)
            {
                auto option = static_cast<rs2_option>(value);
                if (!sensor.supports(option))
                    continue;

                try
                {
                    const auto range = sensor.get_option_range(option);
                    const auto current = sensor.get_option(option);
                    std::cout << "  OPTION " << rs2_option_to_string(option)
                              << " current=" << std::setprecision(9) << current
                              << " range=[" << range.min << "," << range.max << "]"
                              << " step=" << range.step << " default=" << range.def
                              << " read_only=" << sensor.is_option_read_only(option)
                              << "\n";
                }
                catch (const std::exception & error)
                {
                    std::cout << "  OPTION " << rs2_option_to_string(option)
                              << " ERROR=" << error.what() << "\n";
                }
            }

            if (write_depth_units && sensor.supports(RS2_OPTION_DEPTH_UNITS))
            {
                const auto original = sensor.get_option(RS2_OPTION_DEPTH_UNITS);
                sensor.set_option(RS2_OPTION_DEPTH_UNITS, original);
                std::cout << "  WRITE_BACK Depth Units=" << original << " PASS\n";
            }

            if (test_color_auto
                && std::string(sensor.get_info(RS2_CAMERA_INFO_NAME)) == "RGB Camera")
            {
                const auto original_ae = sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE);
                const auto original_exposure = sensor.get_option(RS2_OPTION_EXPOSURE);
                const auto original_gain = sensor.get_option(RS2_OPTION_GAIN);
                const auto original_awb = sensor.get_option(
                    RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);
                const auto original_wb = sensor.get_option(RS2_OPTION_WHITE_BALANCE);

                sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1);
                sensor.set_option(RS2_OPTION_EXPOSURE, original_exposure + 1);
                const bool exposure_disables_ae = sensor.get_option(
                    RS2_OPTION_ENABLE_AUTO_EXPOSURE) == 0;
                sensor.set_option(RS2_OPTION_EXPOSURE, original_exposure);
                sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, original_ae);

                sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1);
                sensor.set_option(RS2_OPTION_GAIN, original_gain + 1);
                const bool gain_disables_ae = sensor.get_option(
                    RS2_OPTION_ENABLE_AUTO_EXPOSURE) == 0;
                sensor.set_option(RS2_OPTION_GAIN, original_gain);
                sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, original_ae);

                sensor.set_option(RS2_OPTION_WHITE_BALANCE, original_wb + 10);
                const bool wb_disables_awb = sensor.get_option(
                    RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE) == 0;
                sensor.set_option(RS2_OPTION_WHITE_BALANCE, original_wb);
                sensor.set_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, original_awb);

                const bool pass = exposure_disables_ae && gain_disables_ae
                    && wb_disables_awb;
                std::cout << "  COLOR_AUTO_DISABLE exposure=" << exposure_disables_ae
                          << " gain=" << gain_disables_ae
                          << " white_balance=" << wb_disables_awb
                          << " restored_ae="
                          << sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE)
                          << " restored_awb="
                          << sensor.get_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE)
                          << " result=" << (pass ? "PASS" : "FAIL") << "\n";
                if (!pass)
                    return 3;
            }
        }
        return 0;
    }
    catch (const std::exception & error)
    {
        std::cerr << "ERROR " << error.what() << "\n";
        return 2;
    }
}
