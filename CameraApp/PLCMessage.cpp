#include "PLCMessage.h"


// 1. Serialize class members into a byte stream (vector of bytes)
std::vector<uint8_t> PLCMessage::serialize() const {
    std::vector<uint8_t> buffer;

    // Serialize 'header_byte1' (Trivial type)
    append_to_buffer(buffer, &header_byte1, sizeof(header_byte1));

    // Serialize 'header_byte2' (Trivial type)
    append_to_buffer(buffer, &header_byte2, sizeof(header_byte2));

    // Serialize 'date_and_time' (struct of trivial types)
    append_to_buffer(buffer, &date_and_time.year, sizeof(date_and_time.year));
    append_to_buffer(buffer, &date_and_time.month, sizeof(date_and_time.month));
    append_to_buffer(buffer, &date_and_time.day, sizeof(date_and_time.day));
    append_to_buffer(buffer, &date_and_time.hour, sizeof(date_and_time.hour));
    append_to_buffer(buffer, &date_and_time.minute, sizeof(date_and_time.minute));
    append_to_buffer(buffer, &date_and_time.second, sizeof(date_and_time.second));
    append_to_buffer(buffer, &date_and_time.milli_high, sizeof(date_and_time.milli_high));
    append_to_buffer(buffer, &date_and_time.milli_low_and_weekday, sizeof(date_and_time.milli_low_and_weekday));

    //Serialize 'messageID' (Trivial type)
    append_to_buffer(buffer, &messageID, sizeof(messageID));

    //Serialize 'equipmentID' (Trivial type)
    append_to_buffer(buffer, &equipmentID, sizeof(equipmentID));

    //Serialize 'equipmentID' (array of Bytes)
    append_to_buffer(buffer, &body, sizeof(body));

    //Serialize 'footer' (Trivial type) x4 times
    append_to_buffer(buffer, &footer1, sizeof(footer1));
    append_to_buffer(buffer, &footer2, sizeof(footer2));
    append_to_buffer(buffer, &footer3, sizeof(footer3));
    append_to_buffer(buffer, &footer4, sizeof(footer4));

    return buffer;
}

// 2. Deserialize bytes back into the class instance
void  PLCMessage::deserialize(const std::vector<uint8_t>& buffer) {
    size_t offset = 0;

    // Deserialize 'header_byte1'
    read_from_buffer(buffer, offset, &header_byte1, sizeof(header_byte1));

    // Deserialize 'header_byte2'
    read_from_buffer(buffer, offset, &header_byte2, sizeof(header_byte2));

    // Deserialize 'date_and_time'
    read_from_buffer(buffer, offset, &date_and_time.year, sizeof(&date_and_time.year));
    read_from_buffer(buffer, offset, &date_and_time.month, sizeof(&date_and_time.month));
    read_from_buffer(buffer, offset, &date_and_time.day, sizeof(&date_and_time.day));
    read_from_buffer(buffer, offset, &date_and_time.hour, sizeof(&date_and_time.hour));
    read_from_buffer(buffer, offset, &date_and_time.minute, sizeof(&date_and_time.minute));
    read_from_buffer(buffer, offset, &date_and_time.second, sizeof(&date_and_time.second));
    read_from_buffer(buffer, offset, &date_and_time.milli_high, sizeof(&date_and_time.milli_high));
    read_from_buffer(buffer, offset, &date_and_time.milli_low_and_weekday, sizeof(&date_and_time.milli_low_and_weekday));

    //Deserialize 'messageID' 
    read_from_buffer(buffer, offset, &messageID, sizeof(&messageID));

    //Desrialize 'equipmentID' 
    read_from_buffer(buffer, offset, &equipmentID, sizeof(&equipmentID));

    //Desrialize 'footer' 
    read_from_buffer(buffer, offset, &footer1, sizeof(&footer1));
    read_from_buffer(buffer, offset, &footer2, sizeof(&footer2));
    read_from_buffer(buffer, offset, &footer3, sizeof(&footer3));
    read_from_buffer(buffer, offset, &footer4, sizeof(&footer4));
}


void PLCMessage::append_to_buffer(std::vector<uint8_t>& buffer, const void* data, size_t size) const {
    const uint8_t* bytePtr = reinterpret_cast<const uint8_t*>(data);
    buffer.insert(buffer.end(), bytePtr, bytePtr + size);
}

// Helper helper to extract raw bytes from the vector
void PLCMessage::read_from_buffer(const std::vector<uint8_t>& buffer, size_t& offset, void* dest, size_t size) {
    if (offset + size > buffer.size()) {
        throw std::runtime_error("Buffer overrun during deserialization!");
    }
    std::memcpy(dest, buffer.data() + offset, size);
    offset += size;
}