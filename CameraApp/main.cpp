
#include "TcpServer.h"
#include "thread_safe_queue.h"
#include "CameraConnector.h"
#include "FrameGrabber.h"
#include "AngleTracking.hpp"
#include "PLCMessage.h"
#include "CameraData.h"
#include "AppConfig.h"
#include "MessageHandler.h"
#include "gui.h"
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

//================Shared data & syncronisation for GUI===============================================
// --   
std::mutex g_frame_mutex;              // Mutex for sync data
cv::Mat g_shared_output_frame;         // Frame shared between threads
bool g_new_frame_available = false;    // Flag to tell UI thread a new frame is ready

// ---  Atomic parameters (GUI Thread <-> OpenCV Thread) 
std::atomic<float> g_param_angle{ 45.0f };
std::atomic<int> g_param_threshold{ 128 };

// --- system LCS control ---
std::atomic<bool> g_app_running{ true }; // Global running state across all threads

//===================================================================================================

int main() {

    //Start GUI proccess
    std::thread gui_thread(
        GuiThreadWorker,
        std::ref(g_frame_mutex),
        std::ref(g_shared_output_frame),
        std::ref(g_new_frame_available),
        std::ref(g_param_angle),
        std::ref(g_param_threshold),
        std::ref(g_app_running)
    );

    

    // 0. Discovery camera devices
    //auto discovered = CameraConnector::DiscoverDevices();
    // if (discovered.empty()) {
    //    WaitForUser();
    //     return EXIT_FAILURE;
    // }

    // Connection to cameras
    // Camera 1
    //...
    //Camera2
    //CameraConnector camera2;
    //if (!camera2.Connect(AppConfig::cam2_ip)) {
    //    WaitForUser();
    //    return EXIT_FAILURE;
    //}

    // Fetch Port
    // Camera 1
    // ...
    // Camera2
    //uint16_t pcic_port_cam2 = camera2.GetPcicPort();
    //std::cout << "Successfully retrieved PCIC Port: " << pcic_port_cam2 << std::endl;

    // Instantiate the FrameGrabber
    // Camera1
    // ...
    // Camera 2
    //FrameGrabber fg_2 = FrameGrabber(camera2.GetDevice(), pcic_port_cam2);

    // Make the chain of PLC messages handlers  
    //auto msg_handler = std::make_shared<MessageHandler>();

    //auto cam1 = std::make_shared<CameraHandler>(fg_1);
    //auto cam2 = std::make_shared<CameraHandler>(fg_2, AppConfig::CameraID::CAMERA_2);

    //msg_handler->set_next(cam2);

    //Start TCP server and threads (workers) for incoming/outcomming messages
    try {
        asio::io_context io_context;

        ThreadSafeQueue<PLCMessage> outbound_pipeline;  //  server  -> PLC
        ThreadSafeQueue<int> inbound_pipeline;          //  PLC -> server 

        Server server(io_context, AppConfig::listnening_port, outbound_pipeline, inbound_pipeline);

        // --- INBOUND CONSUMER THREAD ---
        //  Check data from PLC every 50 ms
        // If new data is avialble -> trigger camera, get pic, calculate relative_angle
        // and push into outbound_pipeline for sending back to PLC

        std::thread inbound_consumer([&inbound_pipeline, &outbound_pipeline]() {
            int received_value = 0;
            while (true) {
                // Poll the queue every 50ms for data from PLC
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
              
                while (inbound_pipeline.try_pop(received_value)) {
                    
                  //  cam2->handle(received_value, outbound_pipeline);

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


    g_app_running = false; // Signals GUI loop to stop
    if (gui_thread.joinable()) gui_thread.join();

    return 0;
}