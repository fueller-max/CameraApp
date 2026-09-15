#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

class Logger {
public:
    explicit Logger(const std::string& filename = "") {
        if (!filename.empty()) {
            logFile.open(filename, std::ios::app);
        }
    }

    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    // Proxy class to capture stream inputs dynamically
    class LogStream {
    public:
        LogStream(Logger& parent, const std::string& level)
            : parentLogger(parent), logLevel(level) {
        }

        // Destructor fires automatically at the semicolon (;), committing the log
        ~LogStream() {
            parentLogger.commitLog(logLevel, stream.str());
        }

        // Overload the << operator to accept ANY streamable type (strings, endpoints, ints, etc.)
        template <typename T>
        LogStream& operator<<(const T& msg) {
            stream << msg;
            return *this;
        }

    private:
        Logger& parentLogger;
        std::string logLevel;
        std::ostringstream stream;
    };

    // This now returns a LogStream proxy object instead of taking a message string
    LogStream log(const std::string& level) {
        return LogStream(*this, level);
    }

private:
    std::ofstream logFile;

    // Internal function called by the LogStream destructor to print out the final message
    void commitLog(const std::string& level, const std::string& message) {
        std::string timestamp = getTimestamp();

        // Clean text entry for the log file
        std::string fileEntry = "[" + timestamp + "] [" + level + "] " + message + "\n";

        // Colorized entry for the console stdout
        std::string colorCode = getColorCode(level);
        std::string resetCode = "\033[0m";
        std::string consoleEntry = "[" + timestamp + "] " + colorCode + "[" + level + "] " + message + resetCode + "\n";

        std::cout << consoleEntry << std::flush;

        if (logFile.is_open()) {
            logFile << fileEntry << std::flush;
        }
    }

    std::string getColorCode(const std::string& level) {
        if (level == "INFO")    return "\033[32m"; // Green
        if (level == "WARNING") return "\033[33m"; // Yellow
        if (level == "ERROR")   return "\033[31m"; // Red
        if (level == "DEBUG")   return "\033[36m"; // Cyan
        return "\033[37m";                         // White
    }

    std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto timeTime = std::chrono::system_clock::to_time_t(now);
        std::tm timeInfo;
#if defined(_MSC_VER)
        localtime_s(&timeInfo, &timeTime);
#else
        localtime_r(&timeTime, &timeInfo);
#endif
        std::ostringstream oss;
        oss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
};


