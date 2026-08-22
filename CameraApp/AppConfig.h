#pragma once


class AppConfig {
public:
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