
#include "message_handler.h"
#include "angle_tracking.h"
#include "main.h"

void CameraHandler::process(int received_value, ThreadSafeQueue<PLCMessage>& outbound_pipeline) {

        std::tuple<ifm3d::Buffer, ifm3d::Buffer> buffer = _fg.Acquire();              // trigger a camera and get a buffered pic
        ifm3d::Buffer ifm_amplitude = std::get<0>(buffer);                            // get amplitude pic from the buffer
        ifm3d::Buffer ifm_distance  = std::get<1>(buffer);                            // get distance pic from the buffer

        cv::Mat amplitude_pic = pictureProcessAndGetAmplitude(ifm_amplitude);               // Amplitude pic for GUI
        cv::Mat distance_pic_human = pictureProcessAndGetDistanceHuman(ifm_distance);       // Distance pic for GUI (colored)
        cv::Mat distance_pic = pictureProcessAndGetDistance(ifm_distance, 
                                                            _g_cam.g_param_distance_mm);    // Distnace pic for CV-analyze

   
         
        //Process the image taken
        std::tuple<cv::Mat, CameraData>  proc_data = processAndFindRotationDist(distance_pic, 
                                                                                _g_cam.g_param_min_area.load(),
                                                                                _g_cam.g_param_max_area.load(),
                                                                                _g_cam.g_param_errosion_size.load(),
                                                                                _g_cam.g_param_dillitation_size.load());
        {  // Update data for GUI
            std::lock_guard<std::mutex> lock(_g_cam.g_frame_mutex);
            _g_cam.g_shared_frame1 = amplitude_pic.clone();           // Amplitude pic
            _g_cam.g_shared_frame2 = distance_pic_human.clone();      // Distance pic
            _g_cam.g_shared_frame3 = std::get<0>(proc_data).clone();  // Calculated(CV) pic                                   
            _g_cam.g_new_frame_available = true;
        }

        // Extract and prepare data for sending to PLC
        CameraData cam_data{ std::get<1>(proc_data).getObjectDetected(), 
                             std::get<1>(proc_data).getRelativeAngle()}; 
        
        PLCMessage plc_message_camera{
            AppConfig::PLCMessage::CAM_DATA_ID,
            static_cast<uint16_t>(_camera_id),
            cam_data.serialize()
        };
        
        outbound_pipeline.push(plc_message_camera);
    }
