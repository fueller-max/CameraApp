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
            cam_data1.g_param_errosion_size = loadedData.cam1.param_errosion_size;
            cam_data1.g_param_dillitation_size = loadedData.cam1.param_dillitation_size;
            cam_data1.g_param_distance_mm = loadedData.cam1.param_distance_mm;

            cam_data2.g_param_max_area = loadedData.cam2.param_max_area;
            cam_data2.g_param_min_area = loadedData.cam2.param_min_area;
            cam_data2.g_param_errosion_size = loadedData.cam2.param_errosion_size;
            cam_data2.g_param_dillitation_size = loadedData.cam2.param_dillitation_size;
            cam_data2.g_param_distance_mm = loadedData.cam2.param_distance_mm;
        }

        return inFile.good();
    }

}
