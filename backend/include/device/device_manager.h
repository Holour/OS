#pragma once

#include "../process/pcb.h"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include "../../libs/json.hpp"

using json = nlohmann::json;

// Represents a single device instance
struct Device {
    int id;
    std::string type;
    std::string name;
    bool is_busy = false;
    std::optional<ProcessID> user_pid; // Which process is using the device
};

class DeviceManager {
public:
    DeviceManager();

    // Request a device of a certain type for a process
    // Returns the device ID if successful, nullopt otherwise.
    std::optional<int> request_device(const std::string& type, ProcessID pid);

    // Release a device being used by a process
    bool release_device(int device_id, ProcessID pid);

    // Perform an operation on a device
    // Returns JSON object with result, or nullopt on failure
    std::optional<json> operate_on_device(int device_id, ProcessID pid, const std::string& operation, const json& params);

    // Get the current state of all devices
    std::vector<Device> get_all_devices() const;

    // Acquire a specific device by device ID for a process
    // Returns the device object if successful, std::nullopt otherwise
    std::optional<Device> acquire_device(int device_id, ProcessID pid);

    // Delete a device from the system. Only allowed when the device is IDLE
    bool delete_device(int device_id);

private:
    std::map<std::string, std::vector<Device>> devices;
    int next_device_id = 0;

    void add_device_type(const std::string& type, int count);
    void add_device(const std::string& type, const std::string& name);
}; 