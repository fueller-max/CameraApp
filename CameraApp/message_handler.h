#pragma once

#include <memory>
#include <iostream>
#include "thread_safe_queue.h"
#include "plc_message.h"
#include "camera_data.h"
#include "frame_grabber.h"


class MessageHandler {
protected:
    std::shared_ptr<MessageHandler> next_handler;

public:
    virtual ~MessageHandler() = default;
    
    void set_next(std::shared_ptr<MessageHandler> next) {
        next_handler = next;
    }

    virtual void handle(int received_value, ThreadSafeQueue<PLCMessage>& outbound_pipeline) {
        if (can_handle(received_value)) {
            process(received_value, outbound_pipeline);
        }
        else if (next_handler) {
            next_handler->handle(received_value, outbound_pipeline);
        }
        else {
            std::cout << "Unhandled request ID: " << received_value << std::endl;
        }
    }

protected:
    virtual bool can_handle(int received_value) const = 0;
    virtual void process(int received_value, ThreadSafeQueue<PLCMessage>& outbound_pipeline) = 0;
};


class CameraHandler : public MessageHandler {
public:
    CameraHandler(FrameGrabber& fg, AppConfig::CameraID camera_id): _fg(fg), _camera_id(camera_id) {}
protected:
    bool can_handle(int received_value) const override {
        return received_value == _camera_id;
    }

    void process(int received_value, ThreadSafeQueue<PLCMessage>& outbound_pipeline) override;
    FrameGrabber& _fg;
    AppConfig::CameraID _camera_id;
};
