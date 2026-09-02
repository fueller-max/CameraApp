#include "tcp_server.h"
#include "thread_safe_queue.h"
#include "camera_connector.h"
#include "frame_grabber.h"
#include "plc_message.h"
#include "camera_data.h"
#include "app_config.h"
#include "message_handler.h"
#include "gui.h"
#include <asio.hpp>
#include <iostream>
#include <chrono>
#include <memory>
#include <condition_variable>

#include "main.h"

using asio::ip::tcp;
using namespace std::chrono_literals;


void static WaitForUser() {
    std::cerr << "Press any key to quit! ";
    std::getchar();
}


int main() {

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
    CameraConnector camera2;
    if (!camera2.Connect(AppConfig::cam2_ip)) {
        WaitForUser();
        return EXIT_FAILURE;
    }

    // Fetch Port
    // Camera 1
    // ...
    // Camera2
    uint16_t pcic_port_cam2 = camera2.GetPcicPort();
    std::cout << "Successfully retrieved PCIC Port: " << pcic_port_cam2 << std::endl;

    // Instantiate the FrameGrabber
    // Camera1
    // ...
    // Camera 2
    FrameGrabber fg_2 = FrameGrabber(camera2.GetDevice(), pcic_port_cam2);

    // Make the chain of PLC messages handlers  
  
    //...
    
    //auto cam1 = std::make_shared<CameraHandler>(fg_1);
    auto cam2 = std::make_shared<CameraHandler>(fg_2, AppConfig::CameraID::CAMERA_2);

    //msg_handler->set_next(cam2);

     //Start GUI proccess
    std::thread gui_thread(
        GuiThreadWorker,
        std::ref(g_frame_mutex),
        std::ref(g_shared_frame1),
        std::ref(g_shared_frame2),
        std::ref(g_shared_frame3),
        std::ref(g_new_frame_available),
        std::ref(g_param_min_area),
        std::ref(g_param_threshold),
        std::ref(g_app_running)
    );

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

        std::thread inbound_consumer([&inbound_pipeline, &outbound_pipeline, &cam2]() {
            int received_value = 0;
            while (true) {
                // Poll the queue every 50ms for data from PLC
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
              
                while (inbound_pipeline.try_pop(received_value)) {
                    
                    cam2->handle(received_value, outbound_pipeline);

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