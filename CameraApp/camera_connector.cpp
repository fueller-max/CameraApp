#include "camera_connector.h"

//Constructor
CameraConnector::CameraConnector(Logger& logger): logger_(logger) {}

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
bool CameraConnector::Connect(const std::string_view ip_address) {

    logger_.log("INFO") << "Trying to connect to camera on IP: " << ip_address;

    try {
        device_ = ifm3d::Device::MakeShared(std::string{ ip_address });
        if (device_) {
            logger_.log("INFO") << "Connection to camera with IP: " << device_->IP() << " was successful!";
            return true;
        }
    }
    catch (const std::exception& e) {

        logger_.log("ERROR") << "Error connecting to device: " << e.what();
    }

    logger_.log("ERROR") << "Failed to create device object."; 
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
        logger_.log("ERROR") << "Error reading PCIC port: " << e.what();
        throw;
    }
}
