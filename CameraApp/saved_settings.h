#pragma once

#include <fstream>
#include <iostream>
#include "camera_gui_data.h"

namespace saved_settings {
    // Data struct for file I/O
    struct SavedSettings {
        int param_max_area;
        int param_min_area;
        int param_errosion_size;
        int param_dillitation_size;
        int param_distance_mm;
    };


    struct DualCameraSettings {
        SavedSettings cam1;
        SavedSettings cam2;
    };

    inline bool g_settings_dirty = false;
    inline float g_save_timer = 0.0f;
    inline const float SAVE_DELAY_THRESHOLD = 1.5f; // Wait 1.5 seconds after the last edit to save
    inline std::string filename = "cam_data.dat";

    void make_settings_dirty();

    bool initializeProgramSettings(CameraGuiData& cam_data1, CameraGuiData& cam_data2);
}


