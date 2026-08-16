#pragma once
#include <cstdint>
#include <iostream>
#include <chrono>



// Packed structure matching Siemens 8-byte DATE_AND_TIME
#pragma pack(push, 1)
struct SiemensDateTime {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t milli_high;
    uint8_t milli_low_and_weekday;
};
#pragma pack(pop)

SiemensDateTime chronoToS7(const std::chrono::system_clock::time_point& tp);

std::chrono::system_clock::time_point s7ToChrono(const SiemensDateTime& dt);