#include "device/device_manager.h"
#include "test_common.h"

// Note: ProcessID is likely a typedef for int or some integer type.
// For the purpose of this test, we'll assume it's `int`.
using ProcessID = int;

void test_initial_device_state() {
    std::cout << "  - Testing Initial Device State..." << std::endl;
    DeviceManager dm;
    auto devices = dm.get_all_devices();
    // Assuming the constructor populates some default devices.
    // Let's check if there are printers and disks as per the API doc example.
    ASSERT_TRUE(devices.size() > 0);
    
    bool found_printer = false;
    bool found_disk = false;
    for (const auto& device : devices) {
        if (device.type == "PRINTER") found_printer = true;
        if (device.type == "DISK") found_disk = true;
        ASSERT_FALSE(device.is_busy);
        ASSERT_FALSE(device.user_pid.has_value());
    }
    ASSERT_TRUE(found_printer);
    ASSERT_TRUE(found_disk);
    std::cout << "    ...PASSED" << std::endl;
}

void test_device_request_and_release() {
    std::cout << "  - Testing Device Request and Release..." << std::endl;
    DeviceManager dm;
    ProcessID p1 = 101;
    ProcessID p2 = 102;

    // P1 requests a printer
    auto dev1_id_opt = dm.request_device("PRINTER", p1);
    ASSERT_TRUE(dev1_id_opt.has_value());
    int dev1_id = dev1_id_opt.value();

    // P2 tries to request the same type, should get a different one if available
    // or fail if not. Let's assume there are multiple printers.
    auto dev2_id_opt = dm.request_device("PRINTER", p2);
    ASSERT_TRUE(dev2_id_opt.has_value());
    ASSERT_NOT_EQUAL(dev1_id, dev2_id_opt.value());

    // Check device states
    auto devices = dm.get_all_devices();
    int busy_printers = 0;
    for (const auto& dev : devices) {
        if (dev.type == "PRINTER" && dev.is_busy) {
            busy_printers++;
            ASSERT_TRUE(dev.user_pid.has_value());
            ASSERT_TRUE(dev.user_pid.value() == p1 || dev.user_pid.value() == p2);
        }
    }
    ASSERT_EQUAL(busy_printers, 2);

    // P1 releases its device
    bool released = dm.release_device(dev1_id, p1);
    ASSERT_TRUE(released);

    // P1 tries to release a device it doesn't own
    released = dm.release_device(dev2_id_opt.value(), p1);
    ASSERT_FALSE(released);

    // Check state again
    auto final_devices = dm.get_all_devices();
    int final_busy_count = 0;
    bool dev1_is_free = false;
    for (const auto& dev : final_devices) {
        if (dev.is_busy) {
            final_busy_count++;
        }
        if (dev.id == dev1_id) {
            dev1_is_free = !dev.is_busy;
        }
    }
    ASSERT_EQUAL(final_busy_count, 1); // Only P2's device should be busy
    ASSERT_TRUE(dev1_is_free);

    std::cout << "    ...PASSED" << std::endl;
}

void test_device_deletion() {
    std::cout << "  - Testing Device Deletion..." << std::endl;
    DeviceManager dm;
    ProcessID p1 = 201;

    // Find a deletable (IDLE) device
    auto devices_before = dm.get_all_devices();
    int device_to_delete = -1;
    for (const auto& dev : devices_before) {
        if (!dev.is_busy) {
            device_to_delete = dev.id;
            break;
        }
    }
    ASSERT_NOT_EQUAL(device_to_delete, -1);

    // Delete it
    bool deleted = dm.delete_device(device_to_delete);
    ASSERT_TRUE(deleted);

    // Verify it's gone
    auto devices_after = dm.get_all_devices();
    ASSERT_EQUAL(devices_after.size(), devices_before.size() - 1);
    for (const auto& dev : devices_after) {
        ASSERT_NOT_EQUAL(dev.id, device_to_delete);
    }
    
    // Try to delete a busy device
    auto busy_dev_id_opt = dm.request_device("DISK", p1);
    ASSERT_TRUE(busy_dev_id_opt.has_value());
    deleted = dm.delete_device(busy_dev_id_opt.value());
    ASSERT_FALSE(deleted);
    
    std::cout << "    ...PASSED" << std::endl;
}


void run_device_manager_tests() {
    test_initial_device_state();
    test_device_request_and_release();
    test_device_deletion();
} 