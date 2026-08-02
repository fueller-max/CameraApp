#include "CameraConnector.h"

// Discovers all available devices on the network
std::vector<ifm3d::IFMNetworkDevice> CameraConnector::DiscoverDevices() {
    auto devices = ifm3d::Device::DeviceDiscovery();

    if (devices.empty()) {
        std::cout << "No devices were found..." << std::endl;
        return devices;
    }

    for (const auto& dev : devices) {
        std::cout << "Found a device! IP: " << dev.GetIPAddress()
            << " Port: " << dev.GetPort() << std::endl;
    }
    return devices;
}

// Establishes connection to a specific camera IP
bool CameraConnector::Connect(const std::string& ip_address) {
    std::cout << "Trying to connect to camera on IP: " << ip_address << "..." << std::endl;

    try {
        device_ = ifm3d::Device::MakeShared(ip_address);
        if (device_) {
            std::cout << "Device object created: " << device_.get() << std::endl;
            return true;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error connecting to device: " << e.what() << std::endl;
    }

    std::cerr << "Failed to create device object." << std::endl;
    device_ = nullptr;
    return false;
}

// Dynamically retrieves the PCIC port from the connected device
uint16_t CameraConnector::GetPcicPort() {
    if (!IsConnected()) {
        throw std::runtime_error("Cannot get PCIC port. Device is not connected.");
    }

    try {
        const std::string port_str = device_->DeviceParameter("PcicTcpPort");
        if (port_str.empty()) {
            throw std::runtime_error("PcicTcpPort parameter is empty.");
        }

        int port_int = std::stoi(port_str);
        if (port_int < 0 || port_int > std::numeric_limits<uint16_t>::max()) {
            throw std::runtime_error("PcicTcpPort out of valid uint16_t range: " + std::to_string(port_int));
        }

        return static_cast<uint16_t>(port_int);
    }
    catch (const std::exception& e) {
        std::cerr << "Error reading PCIC port: " << e.what() << std::endl;
        throw;
    }
}
