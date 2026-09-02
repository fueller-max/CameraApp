#include "camera_data.h"


void CameraData::setObjectDetected() {
	_objectDetected = true;
}
void CameraData::setRelativeAngle(uint16_t relativeAngle) {
	_relativeAngle = relativeAngle;
}
//getters:
bool CameraData::getObjectDetected() const {
	return _objectDetected;
}

int16_t CameraData::getRelativeAngle() const {
	return _relativeAngle;
}

std::vector<uint8_t> CameraData::serialize() const {
    std::vector<uint8_t> buffer;

    // Serialize '_objectDetected' (Trivial type)
    append_to_buffer(buffer, &_objectDetected, sizeof(_objectDetected));
    // Serialize '_objectDetected' (Trivial type)
    append_to_buffer(buffer, &_relativeAngle, sizeof(_relativeAngle));
    return buffer;
};

void CameraData::append_to_buffer(std::vector<uint8_t>& buffer, const void* data, size_t size) const {
    const uint8_t* bytePtr = reinterpret_cast<const uint8_t*>(data);
    if (size > 1) {
        // Insert bytes backwards in case of size more than 1 Byte
        buffer.insert(buffer.end(), std::reverse_iterator<const uint8_t*>(bytePtr + size),
            std::reverse_iterator<const uint8_t*>(bytePtr));
    }
    else {
        // Insert bytes forwards (normal)
        buffer.insert(buffer.end(), bytePtr, bytePtr + size);
    }
}