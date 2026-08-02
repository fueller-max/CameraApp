#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <limits>
#include <ifm3d/device.h>
#include <ifm3d/fg.h>

class CameraConnector {
public:
    CameraConnector() = default;
    ~CameraConnector() = default;

    // Core functionality
    static std::vector<ifm3d::IFMNetworkDevice> DiscoverDevices();
    bool Connect(const std::string& ip_address);
    uint16_t GetPcicPort();

    // Getters
    std::shared_ptr<ifm3d::Device> GetDevice() const { return device_; }
    bool IsConnected() const { return device_ != nullptr; }

private:
    std::shared_ptr<ifm3d::Device> device_{ nullptr };
};