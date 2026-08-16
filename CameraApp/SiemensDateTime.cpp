#include "SiemensDateTime.h"

// Utility: Binary-Coded Decimal (BCD) Helpers
constexpr int bcdToInt(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

constexpr uint8_t intToBcd(int val) {
    return static_cast<uint8_t>(((val / 10) << 4) | (val % 10));
}

// 1. DESERIALIZE: Siemens Bytes -> std::chrono::system_clock::time_point
std::chrono::system_clock::time_point s7ToChrono(const SiemensDateTime& dt) {
    // Unpack BCD fields
    int rawYear = bcdToInt(dt.year);
    int month = bcdToInt(dt.month);
    int day = bcdToInt(dt.day);
    int hour = bcdToInt(dt.hour);
    int minute = bcdToInt(dt.minute);
    int second = bcdToInt(dt.second);

    int msHigh = bcdToInt(dt.milli_high);
    int msLow = (dt.milli_low_and_weekday >> 4) & 0x0F;
    int milli = (msHigh * 10) + msLow;

    // Siemens Year Windowing (90-99 -> 1990-1999, 00-89 -> 2000-2089)
    int year = (rawYear >= 90) ? (1900 + rawYear) : (2000 + rawYear);

    // Populate standard POSIX time structure
    std::tm timeStruct = {};
    timeStruct.tm_year = year - 1900; // std::tm expects years since 1900
    timeStruct.tm_mon = month - 1;   // std::tm expects months 0-11
    timeStruct.tm_mday = day;
    timeStruct.tm_hour = hour;
    timeStruct.tm_min = minute;
    timeStruct.tm_sec = second;
    timeStruct.tm_isdst = -1;         // Let system determine DST setting

    // Convert std::tm to time_t (seconds precision)
    std::time_t timeSecs = std::mktime(&timeStruct);

    // Upgrade to time_point and add sub-second millisecond offset
    auto tp = std::chrono::system_clock::from_time_t(timeSecs);
    return tp + std::chrono::milliseconds(milli);
}

// 2. SERIALIZE: std::chrono::system_clock::time_point -> Siemens Bytes
SiemensDateTime chronoToS7(const std::chrono::system_clock::time_point& tp) {
    // Extract total duration since epoch
    auto duration = tp.time_since_epoch();

    // Isolate sub-second milliseconds
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration) - secs;
    int milli = static_cast<int>(ms.count());

    // Convert time_t baseline into broken-down calendar elements
    std::time_t timeSecs = std::chrono::system_clock::to_time_t(tp);
    std::tm timeStruct;
#if defined(_MSC_VER)
    localtime_s(&timeStruct, &timeSecs); // Windows safe variant
#else
    localtime_r(&timeSecs, &timeStruct); // POSIX safe variant
#endif

    // Format full year back into windowed S7 2-digit format
    int fullYear = timeStruct.tm_year + 1900;
    int rawYear = (fullYear >= 2000) ? (fullYear - 2000) : (fullYear - 1900);

    // Map the 3-digit millisecond value into high/low pieces
    int msHigh = milli / 10; // First two digits (00-99)
    int msLow = milli % 10; // Last single digit (0-9)

    // S7 Weekday format: 1 = Sunday, 2 = Monday, ..., 7 = Saturday
    // std::tm format: 0 = Sunday, 1 = Monday, ..., 6 = Saturday
    int s7Weekday = timeStruct.tm_wday + 1;

    // Pack into raw struct output
    SiemensDateTime dt;
    dt.year = intToBcd(rawYear);
    dt.month = intToBcd(timeStruct.tm_mon + 1);
    dt.day = intToBcd(timeStruct.tm_mday);
    dt.hour = intToBcd(timeStruct.tm_hour);
    dt.minute = intToBcd(timeStruct.tm_min);
    dt.second = intToBcd(timeStruct.tm_sec);
    dt.milli_high = intToBcd(msHigh);
    dt.milli_low_and_weekday = static_cast<uint8_t>((msLow << 4) | (s7Weekday & 0x0F));

    return dt;
}