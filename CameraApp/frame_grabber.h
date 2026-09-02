#pragma once

#include <string>
#include <memory>
#include <ifm3d/device.h>
#include <ifm3d/fg.h>
#include <tuple>


class FrameGrabber {
public:
    FrameGrabber() = default;
    ~FrameGrabber() = default;

    FrameGrabber(std::shared_ptr<ifm3d::Device> device, uint16_t pcic_port) {

        fg_ = std::make_shared<ifm3d::FrameGrabber>(device, pcic_port);

        fg_->Start({ ifm3d::buffer_id::AMPLITUDE_IMAGE, ifm3d::buffer_id::RADIAL_DISTANCE_IMAGE });
    }
    
    // Core functionality
    //take a picture and return amplitude and distance pciture
    std::tuple<ifm3d::Buffer, ifm3d::Buffer> Acquire();

    // Getters
    std::shared_ptr<ifm3d::FrameGrabber> GetFrameFrabber() const { return fg_; }

private:
    std::shared_ptr<ifm3d::FrameGrabber> fg_{ nullptr };
};