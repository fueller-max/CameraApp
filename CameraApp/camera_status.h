#pragma once

#pragma once
#include <cstdint>
#include<vector>

class CameraStatus {

public:
	CameraStatus() = default;

	//setters
	void setCamera1Connected();
	void setCamera2Connected();

	std::vector<uint8_t> serialize() const;

private:
	void append_to_buffer(std::vector<uint8_t>& buffer, const void* data, size_t size) const;
	std::byte cam1_status{};
	std::byte cam2_status{};
};