#include "tcp_server.h"
#include "thread_safe_queue.h"
#include "camera_connector.h"
#include "frame_grabber.h"
#include "plc_message.h"
#include "camera_data.h"
#include "app_config.h"
#include "message_handler.h"
#include "gui.h"
#include "saved_settings.h"
#include "camera_status.h"
#include "Logger.h"
#include <asio.hpp>
#include <iostream>
#include <chrono>
#include <memory>
#include <condition_variable>

#include "main.h"

using asio::ip::tcp;
using namespace std::chrono_literals;


int main() {

    Logger logger("");

    // Discovery camera devices
    //auto discovered = CameraConnector::DiscoverDevices();
    
    // Connection to cameras
    // Camera 1
    CameraConnector camera1(logger);
    if (!camera1.Connect(AppConfig::cam1_ip)) {
        logger.log("ERROR" ) << "Failed to connect to Camera 1";
    }
    //Camera2
    CameraConnector camera2(logger);
    if (!camera2.Connect(AppConfig::cam2_ip)) {
        logger.log("ERROR") << "Failed to connect to Camera 2";
    }

    // Instantiate the FrameGrabber
    // Camera1 
    FrameGrabber fg_1;
    if (camera1.IsConnected()) {
        uint16_t pcic_port_cam1 = camera1.GetPcicPort();
        logger.log("INFO") << "Successfully retrieved Camera 1 PCIC Port:" << pcic_port_cam1;
        fg_1 = FrameGrabber(camera1.GetDevice(), pcic_port_cam1);
    }
    // Camera 2
    FrameGrabber fg_2;
    if (camera2.IsConnected()) {
        uint16_t pcic_port_cam2 = camera2.GetPcicPort();
        logger.log("INFO") << "Successfully retrieved Camera 2 PCIC Port:" << pcic_port_cam2;
        fg_2 = FrameGrabber(camera2.GetDevice(), pcic_port_cam2);
    }
   
    // Camera Handlers
    std::shared_ptr<MessageHandler> cam1 = nullptr;
    std::shared_ptr<MessageHandler> cam2 = nullptr;

    // --- Initialize Camera 1 handler ---
    if (camera1.IsConnected()) {
        g_cam1.is_connected = true;
        cam1 = std::make_shared<CameraHandler>(fg_1, AppConfig::CameraID::CAMERA_1, g_cam1);
    }
    else {
        g_cam1.is_connected = false;
    }

    // --- Initialize Camera 2 handler ---
    if (camera2.IsConnected()) {
        g_cam2.is_connected = true;
        cam2 = std::make_shared<CameraHandler>(fg_2, AppConfig::CameraID::CAMERA_2, g_cam2);
    }
    else {
        g_cam2.is_connected = false;
    }

    // --- Dynamic Chain Linking ---
    std::shared_ptr<MessageHandler> chain_head = nullptr;

    if (cam1 && cam2) {
        cam1->set_next(cam2);
        chain_head = cam1;
    }
    else if (cam1) {
        chain_head = cam1;
    }
    else if (cam2) {
        chain_head = cam2;
    }

    // Load Camera settings from disk 
    saved_settings::initializeProgramSettings(g_cam1, g_cam2);

     //Start GUI proccess
    std::thread gui_thread(
        GuiThreadWorker,
        std::ref(g_cam1),
        std::ref(g_cam2),
        std::ref(g_app_running)
    );

    //Start TCP server and threads (workers) for incoming/outcomming messages
    try {
        asio::io_context io_context;

        ThreadSafeQueue<PLCMessage> outbound_pipeline;  //  server  -> PLC
        ThreadSafeQueue<int> inbound_pipeline;          //  PLC -> server 

        Server server(io_context, AppConfig::listnening_port, outbound_pipeline, inbound_pipeline, logger);

		// --- CAMERA STATUS NOTIFIER THREAD ---
        std::thread camera_status([&outbound_pipeline]() {

            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // sleep for 1 second before sending the status message
                CameraStatus cameras_status;
                if (!g_cam1.is_connected) {
                    cameras_status.setCamera1Connected();
                }
                if (!g_cam2.is_connected) {
                    cameras_status.setCamera2Connected();
                }

                PLCMessage plc_message_camera_status{
                    AppConfig::PLCMessage::CAM_STATUS_ID,
                    AppConfig::EquipmentID::CAM,
                    cameras_status.serialize()
                };
                outbound_pipeline.push(plc_message_camera_status);
            }
            
            });

        // --- INBOUND CONSUMER THREAD ---
        //  Check data from PLC every 50 ms
        // If new data is avialble -> trigger camera, get pic, calculate relative_angle
        // and push into outbound_pipeline for sending back to PLC

        std::thread inbound_consumer([&inbound_pipeline, &outbound_pipeline, chain_head ]() {
            int received_value = 0;
            while (true) {
                // Poll the queue every 50ms for data from PLC
                std::this_thread::sleep_for(std::chrono::milliseconds(50));        
              
                while (inbound_pipeline.try_pop(received_value)) {
                   
                    if (chain_head) {
                        chain_head->handle(received_value, outbound_pipeline);
                    }
                    else {
                        std::cout << "Trigger has been provided but no at least one camera provided" << std::endl;
                    }

                }
            }
            });
        camera_status.detach();
        inbound_consumer.detach();
        logger.log("INFO") << "TCP Server active...";
        io_context.run();

    }
    catch (std::exception& e) {
        logger.log("ERROR") <<"Exception: " << e.what();
    }

    g_app_running = false; // Signals GUI loop to stop
    if (gui_thread.joinable()) gui_thread.join();

    return 0;
}