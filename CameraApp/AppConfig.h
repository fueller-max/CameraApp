#pragma once
#include<string_view>

class AppConfig {
public:

    constexpr static std::string_view cam1_ip = "10.24.150.117";
    constexpr static std::string_view cam2_ip = "10.24.150.118";

    static constexpr int listnening_port = 2030;
    static constexpr int tcp_body_size = 20;

    enum PLCMessage {
        LIFESIGN_ID = 100,
        CAM_DATA_ID = 105
    };
    enum CameraID {
        CAMERA_1 = 10,
        CAMERA_2 = 20
    };
};