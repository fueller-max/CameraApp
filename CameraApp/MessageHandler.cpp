
#include "MessageHandler.h"


void CameraHandler::process(int received_value, ThreadSafeQueue<PLCMessage>& outbound_pipeline) {
        std::cout << " Request for Camera 1 triggering has been provided " << std::endl;
        std::tuple<ifm3d::Buffer, ifm3d::Buffer> buffer = _fg.Acquire();     // trigger a camera and get a buffered pic
        ifm3d::Buffer ifm_amplitude = std::get<0>(buffer);                           // get amplitude pic from the buffer
        ifm3d::Buffer ifm_distance = std::get<1>(buffer);                            // get distance pic from the buffer

        //  int relative_angle = PictureProcessAndGetAngle(ifm_amplitude, ifm_distance); // process with the angle calc

        CameraData cam_data{ true, 0 };
        
        PLCMessage plc_message_camera{
            AppConfig::PLCMessage::CAM_DATA_ID,
            AppConfig::CameraID::CAMERA_1,
            cam_data.serialize()
        };
        
        outbound_pipeline.push(plc_message_camera);
    }




