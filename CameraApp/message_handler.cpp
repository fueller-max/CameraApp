
#include "message_handler.h"
#include "angle_tracking.h"
#include "main.h"

void CameraHandler::process(int received_value, ThreadSafeQueue<PLCMessage>& outbound_pipeline) {
        std::cout << "Request for Camera id:" << _camera_id << " triggering has been provided " << std::endl;

        std::tuple<ifm3d::Buffer, ifm3d::Buffer> buffer = _fg.Acquire();              // trigger a camera and get a buffered pic
        ifm3d::Buffer ifm_amplitude = std::get<0>(buffer);                            // get amplitude pic from the buffer
        ifm3d::Buffer ifm_distance  = std::get<1>(buffer);                            // get distance pic from the buffer

        cv::Mat amplitude_pic = pictureProcessAndGetAmplitude(ifm_amplitude);               // Amplitude for visu
        cv::Mat distance_pic_human = pictureProcessAndGetDistanceHuman(ifm_distance);       // Distance for visu (colored)
        cv::Mat distance_pic = pictureProcessAndGetDistance(ifm_distance);                  //Distnace for analyze

        // Version for amplitude frame
        //std::tuple<cv::Mat, CameraData>  proc_data = processAndFindRotationAmpl(amplitude_pic, g_param_threshold.load(), g_param_min_area.load());
         
        //Version for distance frame
        std::tuple<cv::Mat, CameraData>  proc_data = processAndFindRotationDist(distance_pic, g_param_threshold.load(), g_param_min_area.load());
     
        // Use global variables to update pic for GUI
        {                                                                           
            std::lock_guard<std::mutex> lock(g_frame_mutex);                              
            g_shared_frame1 = amplitude_pic.clone();           // Amplitude pic
            g_shared_frame2 = distance_pic_human.clone();      // Distance pic
            g_shared_frame3 = std::get<0>(proc_data).clone();  // Calculated pic                                   
            g_new_frame_available = true;                                            
        }

        // Extract and prepare data for sending to PLC
        CameraData cam_data{ std::get<1>(proc_data).getObjectDetected(), std::get<1>(proc_data).getRelativeAngle()}; 
        
        PLCMessage plc_message_camera{
            AppConfig::PLCMessage::CAM_DATA_ID,
            static_cast<uint16_t>(_camera_id),
            cam_data.serialize()
        };
        
        outbound_pipeline.push(plc_message_camera);
    }




