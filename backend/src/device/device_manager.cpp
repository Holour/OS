#include "../../include/device/device_manager.h"
#include <iostream>
#include <vector>
#include <string>

DeviceManager::DeviceManager() {
    // 初始化更现实的设备
    // 打印机设备
    add_device("PRINTER", "惠普激光打印机HP-2020");
    add_device("PRINTER", "佳能喷墨打印机Canon-MG3680");
    
    // 音频设备
    add_device("AUDIO", "索尼无线耳机WH-1000XM4");
    add_device("AUDIO", "Bose音响SoundLink Mini");
    add_device("AUDIO", "苹果AirPods Pro");
    add_device("AUDIO", "罗技音箱Z313");
    
    // 其他设备
    add_device("SCANNER", "爱普生扫描仪V600");
    add_device("DISK", "希捷移动硬盘1TB");
    add_device("DISK", "三星SSD 980 PRO");
    
    std::cout << "DeviceManager initialized with realistic devices." << std::endl;
}

void DeviceManager::add_device(const std::string& type, const std::string& name) {
    Device dev;
    dev.id = next_device_id++;
    dev.type = type;
    dev.name = name;
    
    // 如果该类型的设备列表不存在，创建一个
    if (devices.find(type) == devices.end()) {
        devices[type] = std::vector<Device>();
    }
    
    devices[type].push_back(dev);
    std::cout << "Added device: " << name << " (Type: " << type << ", ID: " << dev.id << ")" << std::endl;
}

void DeviceManager::add_device_type(const std::string& type, int count) {
    // 保留原有方法以兼容性，但使用默认名称
    std::vector<Device> device_pool;
    for (int i = 0; i < count; ++i) {
        Device dev;
        dev.id = next_device_id++;
        dev.type = type;
        dev.name = type + std::to_string(i + 1); // 默认名称
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

std::optional<Device> DeviceManager::acquire_device(int device_id, ProcessID pid) {
    for (auto& pair : devices) {
        for (auto& device : pair.second) {
            if (device.id == device_id) {
                if (!device.is_busy) {
                    device.is_busy = true;
                    device.user_pid = pid;
                    return device; // Return copy of the device
                }
                return std::nullopt; // Found but busy
            }
        }
    }
    return std::nullopt; // Not found
}

bool DeviceManager::delete_device(int device_id) {
    for (auto it_type = devices.begin(); it_type != devices.end(); ++it_type) {
        auto& vec = it_type->second;
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            if (it->id == device_id) {
                if (it->is_busy) {
                    return false; // Cannot delete busy device
                }
                vec.erase(it);
                // If no more devices of this type, erase the key entry to keep map clean.
                if (vec.empty()) {
                    devices.erase(it_type);
                }
                return true;
            }
        }
    }
    return false; // Not found
} 