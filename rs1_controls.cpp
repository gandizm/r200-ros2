// SPDX-License-Identifier: Apache-2.0
#include <librealsense/rs.hpp>

#include <iostream>

int main()
{
    try
    {
        rs::context ctx;
        if (ctx.get_device_count() == 0)
            throw std::runtime_error("no RS1 device");
        auto * device = ctx.get_device(0);

        const auto ae = rs::option::color_enable_auto_exposure;
        const auto exposure = rs::option::color_exposure;
        const auto gain = rs::option::color_gain;
        const auto awb = rs::option::color_enable_auto_white_balance;
        const auto wb = rs::option::color_white_balance;

        const double original_ae = device->get_option(ae);
        const double original_exposure = device->get_option(exposure);
        const double original_gain = device->get_option(gain);
        const double original_awb = device->get_option(awb);
        const double original_wb = device->get_option(wb);
        std::cout << "ORIGINAL ae=" << original_ae
                  << " exposure=" << original_exposure
                  << " gain=" << original_gain
                  << " awb=" << original_awb
                  << " wb=" << original_wb << "\n";

        device->set_option(exposure, original_exposure + 1);
        const bool exposure_manual = device->get_option(ae) == 0;
        device->set_option(exposure, original_exposure);
        device->set_option(ae, original_ae);

        device->set_option(gain, original_gain + 1);
        const bool gain_manual = device->get_option(ae) == 0;
        device->set_option(gain, original_gain);
        device->set_option(ae, original_ae);

        device->set_option(wb, original_wb + 10);
        const bool wb_manual = device->get_option(awb) == 0;
        device->set_option(wb, original_wb);
        device->set_option(awb, original_awb);

        const bool pass = exposure_manual && gain_manual && wb_manual;
        std::cout << "RS1_COLOR_AUTO_DISABLE exposure=" << exposure_manual
                  << " gain=" << gain_manual
                  << " white_balance=" << wb_manual
                  << " restored_ae=" << device->get_option(ae)
                  << " restored_awb=" << device->get_option(awb)
                  << " result=" << (pass ? "PASS" : "FAIL") << "\n";
        return pass ? 0 : 1;
    }
    catch (const std::exception & error)
    {
        std::cerr << "ERROR " << error.what() << "\n";
        return 2;
    }
}
