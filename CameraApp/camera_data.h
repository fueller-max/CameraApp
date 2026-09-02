#pragma once
#include <cstdint>
#include<vector>

class CameraData {

public:
	CameraData() = default;
	CameraData(bool objectDetected, int16_t relativeAngle) : _objectDetected(objectDetected), _relativeAngle{relativeAngle}{}

	//setters
	void setObjectDetected();
	void setRelativeAngle(uint16_t relativeAngle);
	//getters:
	bool getObjectDetected() const;
	int16_t getRelativeAngle() const;

	std::vector<uint8_t> serialize() const;

private:
	void append_to_buffer(std::vector<uint8_t>& buffer, const void* data, size_t size) const;
	bool _objectDetected = false;
	int16_t _relativeAngle = 0;
};