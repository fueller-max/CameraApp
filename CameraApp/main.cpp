
#include "TcpServer.h"
#include "thread_safe_queue.h"
#include "CameraConnector.h"
#include "FrameGrabber.h"
#include "AngleTracking.hpp"
#include "PLCMessage.h"
#include <asio.hpp>
#include <iostream>
#include <chrono>
#include <memory>
#include <condition_variable>
#include <opencv2/opencv.hpp>

using asio::ip::tcp;
using namespace std::chrono_literals;


void WaitForUser() {
    std::cerr << "Press any key to quit! ";
    std::getchar();
}

int PictureProcessAndGetAngle(ifm3d::Buffer ifm_amplitude, ifm3d::Buffer ifm_distance) {

    //Map ifm3d data structures into OpenCV cv::Mat containers
    // O3D303 natively uses 16-bit unsigned integers (CV_16UC1) for ToF pixel maps
    cv::Mat amplitude_mat(ifm_amplitude.Height(), ifm_amplitude.Width(), CV_16UC1, ifm_amplitude.Ptr<uint16_t>(0));
    cv::Mat distance_mat(ifm_distance.Height(), ifm_distance.Width(), CV_16UC1, ifm_distance.Ptr<uint16_t>(0));

    // Data Preparation for Visualization (Convert 16-bit to 8-bit grayscale)
    cv::Mat amplitude_visual, distance_visual;

    // Normalize Amplitude: Maps IR return strength to a readable 0-255 spectrum
    cv::normalize(amplitude_mat, amplitude_visual, 0, 255, cv::NORM_MINMAX, CV_8UC1);

    // Normalize Distance: Maps structural depths to a readable 0-255 spectrum
    cv::normalize(distance_mat, distance_visual, 0, 255, cv::NORM_MINMAX, CV_8UC1);
    // Optional: Apply a colormap to the distance map for clear depth visualization
    cv::Mat distance_color;
    cv::applyColorMap(distance_visual, distance_color, cv::COLORMAP_JET);

    //CV-processing
     int angle = processAndFindRotation(amplitude_mat);
     return angle;
    
}

int main() {

    // 0. Discovery camera devices
    //auto discovered = CameraConnector::DiscoverDevices();
    // if (discovered.empty()) {
    //    WaitForUser();
    //     return EXIT_FAILURE;
    // }

    // 1. Connection routine 
   // CameraConnector camera;
   // const std::string camera_ip = "10.24.150.118";

   // if (!camera.Connect(camera_ip)) {
   //     WaitForUser();
   //     return EXIT_FAILURE;
  //  }

    // 2. Fetch Port
   //  uint16_t pcic_port = camera.GetPcicPort();
  //   std::cout << "Successfully retrieved PCIC Port: " << pcic_port << std::endl;
    
    // 3. Instantiate the FrameGrabber
     //FrameGrabber fg = FrameGrabber(camera.GetDevice(), pcic_port);

     //std::tuple<ifm3d::Buffer, ifm3d::Buffer> buffer = fg.Acquire();

    // auto ifm_amplitude = std::get<0>(buffer);  // get amplitude the from buffer
    // auto ifm_distance = std::get<1>(buffer);   // get distance from the buffer

     //PictureProcess(ifm_amplitude, ifm_distance);


    //Start TCP server and threads (workers) for incoming/outcomming messages
    try {
        asio::io_context io_context;

        ThreadSafeQueue<PLCMessage> outbound_pipeline; //  server  -> clients
        ThreadSafeQueue<int> inbound_pipeline;  //  clients -> server 

        Server server(io_context, 2030, outbound_pipeline, inbound_pipeline);

        // --- INBOUND CONSUMER THREAD ---
        //  Check data from PLC every 50 ms
        // If new data is avialble -> trigger camera, get pic, calculate relative_angle
        // and push into outbound_pipeline for sending back to PLC
      
        // std::thread inbound_consumer([&inbound_pipeline, &outbound_pipeline, &fg]() {
        std::thread inbound_consumer([&inbound_pipeline, &outbound_pipeline]() {
            int received_value = 0;
            while (true) {
                // Poll the queue every 50ms for data from PLC
                std::this_thread::sleep_for(std::chrono::milliseconds(5000));

                PLCMessage  plc_message_outgoing {};

                outbound_pipeline.push(plc_message_outgoing);

                while (inbound_pipeline.try_pop(received_value)) {

                    if (received_value == 10) {
                        std::cout << " Request for Camera 1 triggering has been provided " << std::endl;
                     
                    }

                    if (received_value == 20) {
                        std::cout << " Request for Camera 2 triggering has been provided " << std::endl;
                
                        //std::tuple<ifm3d::Buffer, ifm3d::Buffer> buffer = fg.Acquire();

                       // auto ifm_amplitude = std::get<0>(buffer);  // get amplitude from the buffer
                       // auto ifm_distance = std::get<1>(buffer);   // get distance from the buffer

                      //  int relative_angle = PictureProcessAndGetAngle(ifm_amplitude, ifm_distance);

                        //outbound_pipeline.push(relative_angle);

                    }
                }
            }
            });
        inbound_consumer.detach();

        std::cout << "TCP Server active..." << std::endl;
        io_context.run();

    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}