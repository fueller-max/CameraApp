
#include "message_handler.h"
#include "angle_tracking.h"
#include "main.h"

void CameraHandler::process(int received_value, ThreadSafeQueue<PLCMessage>& outbound_pipeline) {
        std::cout << "Request for Camera id:" << _camera_id << " triggering has been provided " << std::endl;

        std::tuple<ifm3d::Buffer, ifm3d::Buffer> buffer = _fg.Acquire();              // trigger a camera and get a buffered pic
        ifm3d::Buffer ifm_amplitude = std::get<0>(buffer);                            // get amplitude pic from the buffer
        ifm3d::Buffer ifm_distance  = std::get<1>(buffer);                            // get distance pic from the buffer
        
        std::cout << "Get amplutude picture from camera Camera id:" << _camera_id << std::endl;

        std::cout << " Width: " << ifm_amplitude.Width() <<  _camera_id << std::endl;
        std::cout << " Height: " << ifm_amplitude.Width() << _camera_id << std::endl;

        cv::Mat amplitude_pic = pictureProcessAndGetAmplitude(ifm_amplitude);
        cv::Mat distance_pic = pictureProcessAndGetDistance(ifm_distance);

        std::tuple<cv::Mat, CameraData>  proc_data = processAndFindRotation(amplitude_pic, g_param_threshold.load(), g_param_min_area.load());
         
        // Use global variables to update pic for GUI
        {                                                                           
            std::lock_guard<std::mutex> lock(g_frame_mutex);                              
            g_shared_frame1 = amplitude_pic.clone();           // Amplitude pic
            g_shared_frame2 = distance_pic.clone();            // Distance pic
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




