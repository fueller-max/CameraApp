#include "FrameGrabber.h"
#include <chrono>
#include <memory>


using namespace std::chrono_literals;
// Core functionality
  //take a picture
std::tuple<ifm3d::Buffer, ifm3d::Buffer> FrameGrabber::Acquire() {

    fg_->SWTrigger();

    // Fetch a frame from the framegrabber with a 3-second timeout
    auto future = fg_->WaitForFrame();

    if (future.wait_for(3s) != std::future_status::ready) {
        std::cerr << "Timeout waiting for new data from O3D303!" << std::endl;
    }

    auto frame = future.get();

    //Acquiring data from the device
    return { frame->GetBuffer(ifm3d::buffer_id::AMPLITUDE_IMAGE),
             frame->GetBuffer(ifm3d::buffer_id::RADIAL_DISTANCE_IMAGE) };
}