#include "../../include/device/device_manager.h"
#include <iostream>

DeviceManager::DeviceManager() {
    // Initialize the system with some devices
    add_device_type("printer", 2); // 2 printers
    add_device_type("scanner", 1); // 1 scanner
    std::cout << "DeviceManager initialized with printers and a scanner." << std::endl;
}

void DeviceManager::add_device_type(const std::string& type, int count) {
    std::vector<Device> device_pool;
    for (int i = 0; i < count; ++i) {
        Device dev;
        dev.id = next_device_id++;
        dev.type = type;
        device_pool.push_back(dev);
    }
    devices[type] = device_pool;
}

std::optional<int> DeviceManager::request_device(const std::string& type, ProcessID pid) {
    if (devices.find(type) == devices.end()) {
        return std::nullopt; // Device type not found
    }

    for (auto& device : devices[type]) {
        if (!device.is_busy) {
            device.is_busy = true;
            device.user_pid = pid;
            return device.id;
        }
    }

    return std::nullopt; // All devices of this type are busy
}

bool DeviceManager::release_device(int device_id, ProcessID pid) {
    for (auto& pair : devices) {
        for (auto& device : pair.second) {
            if (device.id == device_id) {
                if (device.is_busy && device.user_pid == pid) {
                    device.is_busy = false;
                    device.user_pid = std::nullopt;
                    return true;
                }
                // Device found, but not used by this process or already free
                return false;
            }
        }
    }
    return false; // Device ID not found
}

std::optional<json> DeviceManager::operate_on_device(int device_id, ProcessID pid, const std::string& operation, const json& params) {
    Device* target_device = nullptr;
    for (auto& pair : devices) {
        for (auto& device : pair.second) {
            if (device.id == device_id) {
                target_device = &device;
                break;
            }
        }
        if (target_device) break;
    }

    if (!target_device) {
        // Device ID not found
        return std::nullopt;
    }

    if (!target_device->is_busy || target_device->user_pid != pid) {
        // Device is not busy or not owned by the requesting process
        return std::nullopt;
    }

    if (operation == "PRINT") {
        if (!params.contains("file_path")) {
            return std::nullopt; // Missing required parameter
        }
        // In a real scenario, we would queue a print job. Here, we just simulate success.
        json result;
        result["job_id"] = "PRINT_" + std::to_string(rand());
        result["estimated_time"] = 30; // 30 seconds
        return result;
    } else {
        // Unknown operation
        return std::nullopt;
    }
}

std::vector<Device> DeviceManager::get_all_devices() const {
    std::vector<Device> all;
    for (const auto& pair : devices) {
        all.insert(all.end(), pair.second.begin(), pair.second.end());
    }
    return all;
} 