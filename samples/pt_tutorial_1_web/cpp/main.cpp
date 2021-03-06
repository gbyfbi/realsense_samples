// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <signal.h>

#include <librealsense/rs.hpp>
#include "rs_sdk.h"
#include "version.h"
#include "pt_utils.hpp"
#include "pt_console_display.hpp"
#include "pt_web_display.hpp"

using namespace std;

// Version number of the samples
extern constexpr auto rs_sample_version = concat("VERSION: ",RS_SAMPLE_VERSION_STR);

unique_ptr<web_display::pt_web_display>      web_view;
unique_ptr<console_display::pt_console_display>    console_view;


int main(int argc, char** argv)
{

    pt_utils pt_utils;

    rs::core::video_module_interface::actual_module_config actualModuleConfig;
    rs::person_tracking::person_tracking_video_module_interface* ptModule = nullptr;


    // Initialize camera and PT module
    if(pt_utils.init_camera(actualModuleConfig) != rs::core::status_no_error)
    {
        cerr << "Error: Device is null." << endl << "Please connect a RealSense device and restart the application" << endl;
        return -1;
    }
    pt_utils.init_person_tracking(&ptModule);

    // Enable Person Tracking module
    ptModule->QueryConfiguration()->QueryTracking()->Enable();
    ptModule->QueryConfiguration()->QueryTracking()->SetTrackingMode((Intel::RealSense::PersonTracking::PersonTrackingConfiguration::TrackingConfiguration::TrackingMode)0);

    // Configure enabled Person Tracking module
    if(ptModule->set_module_config(actualModuleConfig) != rs::core::status_no_error)
    {
        cerr<<"Error : Failed to configure the enabled Person Tracking module" << endl;
        return -1;
    }

    // Start the camera
    pt_utils.start_camera();

    // Launch GUI
    string sample_name = argv[0];

    // Create and start remote(Web) view
    web_view = move(web_display::make_pt_web_display(sample_name, 8000, true));
    // Create console view
    console_view = move(console_display::make_console_pt_display());

    cout << endl << "-------- Press Esc key to exit --------" << endl << endl;
    while(!pt_utils.user_request_exit())
    {
        rs::core::correlated_sample_set sampleSet = {};
        int cumulativeTotal=0;

        // Get next frame
        if (pt_utils.GetNextFrame(sampleSet) != 0)
        {
            cerr << "Error: Invalid frame" << endl;
            continue;
        }

        // Process frame
        if (ptModule->process_sample_set(sampleSet) != rs::core::status_no_error)
        {
            cerr << "Error : Failed to process sample" << endl;
            continue;
        }

        auto colorImage = sampleSet[rs::core::stream_type::color];

        // Sending dummy time stamp of 10.
        // Display number of persons in the current frame and cumulative total in the GUI.
        web_view->on_rgb_frame(10, colorImage->query_info().width, colorImage->query_info().height, colorImage->query_data());
        cumulativeTotal = console_view->on_person_count_update(ptModule);
        web_view->on_PT_tracking_update(ptModule, cumulativeTotal);

        // Release color and depth image
        sampleSet.images[static_cast<uint8_t>(rs::core::stream_type::color)]->release();
        sampleSet.images[static_cast<uint8_t>(rs::core::stream_type::depth)]->release();

    }

    pt_utils.stop_camera();
    actualModuleConfig.projection->release();
    cout << "-------- Stopping --------" << endl;
    return 0;
}
