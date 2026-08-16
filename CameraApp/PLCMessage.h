#pragma once
#include <cstdint>
#include "SiemensDateTime.h"
#include <vector>

class PLCMessage {
public:
    PLCMessage() = default;
    ~PLCMessage() = default;

    std::vector<uint8_t> serialize() const;
    void  deserialize(const std::vector<uint8_t>& buffer);

private:

    // Helper helper to push raw bytes into the vector
    void append_to_buffer(std::vector<uint8_t>&buffer, const void* data, size_t size) const;
    void read_from_buffer(const std::vector<uint8_t>& buffer, size_t& offset, void* dest, size_t size);

    unsigned char header_byte1{ 0x05 };
    unsigned char header_byte2{ 0x00 };
    SiemensDateTime date_and_time;
    int messageID;
    int equipmentID;
    unsigned char body[268] ;
    unsigned char footer1{ 0xFF };
    unsigned char footer2{ 0xFF };
    unsigned char footer3{ 0xFF };
    unsigned char footer4{ 0xFF };
};