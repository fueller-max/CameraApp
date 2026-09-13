#include "saved_settings.h"

namespace saved_settings {

    void make_settings_dirty() {
        g_settings_dirty = true;
        g_save_timer = 0.0f;
    }

    bool initializeProgramSettings(CameraGuiData& cam_data1, CameraGuiData& cam_data2) {
        DualCameraSettings loadedData;
        std::ifstream inFile(filename, std::ios::binary);

        // If file exists -> take data from file an write into g_cam data
        // If not -> do nothing, the values are being initalized by structure`s default values
        if (inFile) {
 
            inFile.read(reinterpret_cast<char*>(&loadedData), sizeof(DualCameraSettings));

            cam_data1.g_param_max_area = loadedData.cam1.param_max_area;
            cam_data1.g_param_min_area = loadedData.cam1.param_min_area;
            cam_data1.g_param_errosion_hor = loadedData.cam1.param_errosion_hor;
            cam_data1.g_param_errosion_vert = loadedData.cam1.param_errosion_vert;
            cam_data1.g_param_dilitation_hor = loadedData.cam1.param_dilitation_hor;
            cam_data1.g_param_dilitation_vert = loadedData.cam1.param_dilitation_vert;
            cam_data1.g_param_threshold = loadedData.cam1.param_threshold;

            cam_data2.g_param_max_area = loadedData.cam2.param_max_area;
            cam_data2.g_param_min_area = loadedData.cam2.param_min_area;
            cam_data2.g_param_errosion_hor = loadedData.cam2.param_errosion_hor;
            cam_data2.g_param_errosion_vert = loadedData.cam2.param_errosion_vert;
            cam_data2.g_param_dilitation_hor = loadedData.cam2.param_dilitation_hor;
            cam_data2.g_param_dilitation_vert = loadedData.cam2.param_dilitation_vert;
            cam_data2.g_param_threshold = loadedData.cam2.param_threshold;
        }

        return inFile.good();
    }

}
