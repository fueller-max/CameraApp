#include "camera_status.h"


std::vector<uint8_t> CameraStatus::serialize() const {
    std::vector<uint8_t> buffer;

    // Serialize 'objectDetected' (Trivial type)
    append_to_buffer(buffer, &cam1_status, sizeof(cam1_status));
    // Serialize 'relative angle' (Trivial type)
    append_to_buffer(buffer, &cam2_status, sizeof(cam1_status));
    return buffer;
};

void CameraStatus::append_to_buffer(std::vector<uint8_t>& buffer, const void* data, size_t size) const {
    const uint8_t* bytePtr = reinterpret_cast<const uint8_t*>(data);
    buffer.insert(buffer.end(), bytePtr, bytePtr + size);
   
}

void CameraStatus::setCamera1Connected() {
    cam1_status |= std::byte{ 1 } << 0;   // Set bit 0 - Camera connected
   
};

void CameraStatus::setCamera2Connected() {
    cam2_status |= std::byte{ 1 } << 0;   // Set bit 0 - Camera connected
};