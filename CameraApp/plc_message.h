#pragma once
#include <cstdint>
#include "SiemensDateTime.h"
#include <vector>
#include "app_config.h"

class PLCMessage {
public:
    PLCMessage() = default;
    ~PLCMessage() = default;

    PLCMessage(uint16_t messageID, uint16_t equipmentID, std::vector<uint8_t> body) {
        set_time(); // set time 
        set_body(body); // set body
        this->messageID = messageID;
        this->equipmentID = equipmentID;
    }

    std::vector<uint8_t> serialize() const;
    void  deserialize(const std::vector<uint8_t>& buffer);

private:

    // Helper helper to push raw bytes into the vector
    void append_to_buffer(std::vector<uint8_t>&buffer, const void* data, size_t size) const;
    void read_from_buffer(const std::vector<uint8_t>& buffer, size_t& offset, void* dest, size_t size);

    //Set current time
    void set_time();
    void set_body(std::vector<uint8_t> body);

    unsigned char header_byte1{ 0x05 };
    unsigned char header_byte2{ 0x00 };
    SiemensDateTime date_and_time;
    uint16_t messageID;
    uint16_t equipmentID;
    unsigned char body[AppConfig::tcp_body_size] ;
    unsigned char footer1{ 0xFF };
    unsigned char footer2{ 0xFF };
    unsigned char footer3{ 0xFF };
    unsigned char footer4{ 0xFF };
};