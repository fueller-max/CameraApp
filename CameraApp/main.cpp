
#include "tcp_server.h"
#include "thread_safe_queue.h"
#include "CameraConnector.h"
#include "angle_tracking.hpp"
#include <asio.hpp>
#include <iostream>
#include <chrono>
#include <memory>
#include <opencv2/opencv.hpp>

using asio::ip::tcp;
using namespace std::chrono_literals;


void WaitForUser() {
    std::cerr << "Press any key to quit! ";
    std::getchar();
}

void TriggerCamera() {
    std::cout << " [Action] Triggering hardware camera..." << std::endl;
}

int main() {

    // 0. Discovery camera devices
    //auto discovered = CameraConnector::DiscoverDevices();
   /// if (discovered.empty()) {
    //    WaitForUser();
   //     return EXIT_FAILURE;
   // }

    // 1. Connection routine 
    CameraConnector camera;
    const std::string camera_ip = "10.24.150.118";

    if (!camera.Connect(camera_ip)) {
        WaitForUser();
        return EXIT_FAILURE;
    }

    // 2. Fetch Port
    uint16_t pcic_port = camera.GetPcicPort();
    std::cout << "Successfully retrieved PCIC Port: " << pcic_port << std::endl;
  

    // 3. Instantiate the FrameGrabber
    auto fg = std::make_shared<ifm3d::FrameGrabber>(camera.GetDevice(), pcic_port);

    // 4. Request Amplitude (grayscale) and Radial Distance images from the hardware
    fg->Start({ ifm3d::buffer_id::AMPLITUDE_IMAGE, ifm3d::buffer_id::RADIAL_DISTANCE_IMAGE });

    fg->SWTrigger();

    // Fetch a frame from the framegrabber with a 300-second timeout
    auto future = fg->WaitForFrame();

    if (future.wait_for(3s) != std::future_status::ready) {
        std::cerr << "Timeout waiting for new data from O3D303!" << std::endl;
    }

    auto frame = future.get();

    // 5.  Acquiring data from the device
    auto ifm_amplitude =
        frame->GetBuffer(ifm3d::buffer_id::AMPLITUDE_IMAGE);
    auto ifm_distance =
        frame->GetBuffer(ifm3d::buffer_id::RADIAL_DISTANCE_IMAGE);
    
    // 6. Map ifm3d data structures into OpenCV cv::Mat containers
        // O3D303 natively uses 16-bit unsigned integers (CV_16UC1) for ToF pixel maps
    cv::Mat amplitude_mat(ifm_amplitude.Height(), ifm_amplitude.Width(), CV_16UC1, ifm_amplitude.Ptr<uint16_t>(0));
    cv::Mat distance_mat(ifm_distance.Height(), ifm_distance.Width(), CV_16UC1, ifm_distance.Ptr<uint16_t>(0));

    // 7. Data Preparation for Visualization (Convert 16-bit to 8-bit grayscale)
    cv::Mat amplitude_visual, distance_visual;

    // Normalize Amplitude: Maps IR return strength to a readable 0-255 spectrum
    cv::normalize(amplitude_mat, amplitude_visual, 0, 255, cv::NORM_MINMAX, CV_8UC1);

    // Normalize Distance: Maps structural depths to a readable 0-255 spectrum
    cv::normalize(distance_mat, distance_visual, 0, 255, cv::NORM_MINMAX, CV_8UC1);
    // Optional: Apply a colormap to the distance map for clear depth visualization
    cv::Mat distance_color;
    cv::applyColorMap(distance_visual, distance_color, cv::COLORMAP_JET);

    // ==========================================
    // 8.   Custom CV / Processing Here
    // ==========================================
     
    processAndFindRotation(amplitude_mat);

    //Start TCP server and threads (workers) for incoming/outcomming messages
    try {
        asio::io_context io_context;


        ThreadSafeQueue<int> outbound_pipeline; //  server  -> clients
        ThreadSafeQueue<int> inbound_pipeline;  //  clients -> server 

        Server server(io_context, 2030, outbound_pipeline, inbound_pipeline);

        // --- OUTBOUND PRODUCER THREAD ---
        // Data to PLC
        int external_angle = 25;
        std::thread outbound_producer([&outbound_pipeline, &external_angle]() {
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                outbound_pipeline.push(external_angle);
            }
            });
        outbound_producer.detach();

        // --- INBOUND CONSUMER THREAD ---
        //  Data from PLC
      
        std::thread inbound_consumer([&inbound_pipeline, &fg]() {
            int received_value = 0;
            while (true) {
                // Poll the queue every 50ms for data from PLC
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                while (inbound_pipeline.try_pop(received_value)) {
                    std::cout << " Displaying data from CPU: "
                        << received_value << std::endl;

                    if (received_value == 10) {
                        std::cout << " Request for Camera 1 triggering has been provided " << std::endl;
                     
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