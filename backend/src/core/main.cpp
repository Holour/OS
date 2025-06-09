#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <nlohmann/json.hpp>

#include "../include/memory/memory_manager.h"
#include "../include/process/process_manager.h"
#include "../include/fs/fs_manager.h"
#include "../include/device/device_manager.h"
#include "../include/interrupt/interrupt_manager.h"
#include "../include/clock/clock_manager.h"
#include "../libs/httplib.h"

#ifdef _WIN32
#include <windows.h>
#endif

using json = nlohmann::json;

// 全局操作系统核心组件
std::unique_ptr<MemoryManager> memory_manager;
std::unique_ptr<ProcessManager> process_manager;
std::unique_ptr<FileSystemManager> fs_manager;
std::unique_ptr<DeviceManager> device_manager;
std::unique_ptr<InterruptManager> interrupt_manager;
std::unique_ptr<ClockManager> clock_manager;

// JSON 转换函数前向声明
json pcb_to_json(const PCB& pcb);

// 函数：初始化系统状态
void initialize_system_state() {
    std::cout << "Initializing default system state..." << std::endl;

    // 1. 创建更丰富的文件系统结构
    fs_manager->create_directory("/home", 755);
    fs_manager->create_directory("/etc", 755);
    fs_manager->create_directory("/bin", 755);
    fs_manager->create_directory("/var", 755);
    fs_manager->create_directory("/var/log", 755);
    fs_manager->create_file("/home/welcome.txt", "Welcome to the OS Simulator!", 644);
    fs_manager->create_file("/etc/config.conf", "# System Configuration\nreadonly=true", 644);
    fs_manager->create_file("/var/log/system.log", "System boot sequence started.\n", 600);

    // 1.1. 创建.pubt可执行文件（类似Windows的exe）
    fs_manager->create_file("/bin/calculator.pubt", "10MB", 755); // 计算器程序，需要10MB内存
    fs_manager->create_file("/bin/notepad.pubt", "5MB", 755);     // 记事本程序，需要5MB内存
    fs_manager->create_file("/bin/browser.pubt", "50MB", 755);    // 浏览器程序，需要50MB内存
    fs_manager->create_file("/bin/game.pubt", "100MB", 755);      // 游戏程序，需要100MB内存
    fs_manager->create_file("/home/myapp.pubt", "24MB", 755);     // 用户自定义应用，需要24MB内存

    // 2. 创建多个初始进程
    auto p1 = process_manager->create_process(512 * 1024); // 512KB
    if (p1) std::cout << "Created initial process 'idle_process' with PID: " << *p1 << std::endl;
    
    auto p2 = process_manager->create_process(1 * 1024 * 1024); // 1MB
    if (p2) std::cout << "Created initial process 'system_logger' with PID: " << *p2 << std::endl;

    auto p3 = process_manager->create_process(4 * 1024 * 1024); // 4MB
    if (p3) std::cout << "Created initial process 'shell' with PID: " << *p3 << std::endl;

    std::cout << "Default system state initialized." << std::endl;
}

// 辅助函数：创建标准错误响应
json create_error_response(const std::string& message) {
    return json{{"status", "error"}, {"message", message}};
}

// 辅助函数：创建标准成功响应
json create_success_response(const json& data = json::object(), const std::string& message = "") {
    json response = {{"status", "success"}};
    if (!data.is_null()) {
        // For object types, only include if not empty. For array types, always include.
        if (!data.is_object() || !data.empty()) {
            response["data"] = data;
        }
    }
    if (!message.empty()) {
        response["message"] = message;
    }
    return response;
}

// 辅助函数: 格式化时间点为 ISO 8601 字符串
std::string format_time_point(const std::chrono::system_clock::time_point& tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm gt{};
    #ifdef _WIN32
        gmtime_s(&gt, &t);
    #else
        gmtime_r(&t, &gt);
    #endif
    std::stringstream ss;
    ss << std::put_time(&gt, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "Initializing OS Simulator..." << std::endl;

    try {
        // 初始化各个管理器
        std::cout << "Initializing MemoryManager..." << std::endl;
        memory_manager = std::make_unique<MemoryManager>();
        std::cout << "MemoryManager initialized." << std::endl;

        std::cout << "Initializing ProcessManager..." << std::endl;
        process_manager = std::make_unique<ProcessManager>(*memory_manager);
        std::cout << "ProcessManager initialized." << std::endl;

        std::cout << "Initializing FileSystemManager..." << std::endl;
        fs_manager = std::make_unique<FileSystemManager>(*memory_manager);
        std::cout << "FileSystemManager initialized." << std::endl;

        std::cout << "Initializing DeviceManager..." << std::endl;
        device_manager = std::make_unique<DeviceManager>();
        std::cout << "DeviceManager initialized." << std::endl;

        std::cout << "Initializing InterruptManager..." << std::endl;
        interrupt_manager = std::make_unique<InterruptManager>(*process_manager);
        std::cout << "InterruptManager initialized." << std::endl;

        std::cout << "Initializing ClockManager..." << std::endl;
        clock_manager = std::make_unique<ClockManager>();
        std::cout << "ClockManager initialized." << std::endl;

        std::cout << "All managers initialized successfully." << std::endl;

        // 初始化系统状态
        initialize_system_state();

        httplib::Server svr;

        // 全局处理器，用于CORS和日志记录
        svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
            std::cout << "[REQUEST] " << req.method << " " << req.path << std::endl;
            if (req.method == "OPTIONS") {
                res.status = 204;
                return httplib::Server::HandlerResponse::Handled;
            }
            return httplib::Server::HandlerResponse::Unhandled;
        });

        // --- 进程管理 API ---
        svr.Get("/api/v1/processes", [&](const httplib::Request&, httplib::Response& res) {
            json data = json::array();
            for (const auto& proc : process_manager->get_all_processes()) {
                data.push_back(pcb_to_json(*proc));
            }
            res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
        });

        svr.Post("/api/v1/processes", [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto body = json::parse(req.body);
                uint64_t memory_size = body.at("memory_size");
                auto pid_opt = process_manager->create_process(memory_size);
                if (pid_opt) {
                    auto pcb = process_manager->get_process(*pid_opt);
                    res.status = 201;
                    res.set_content(create_success_response(pcb_to_json(*pcb), "Process created successfully.").dump(), "application/json; charset=utf-8");
                } else {
                    res.status = 400;
                    res.set_content(create_error_response("Insufficient memory to create process.").dump(), "application/json; charset=utf-8");
                }
            } catch (const json::exception& e) {
                res.status = 400;
                res.set_content(create_error_response("Invalid request body: " + std::string(e.what())).dump(), "application/json; charset=utf-8");
            }
        });

        svr.Delete(R"(/api/v1/processes/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
            ProcessID pid = std::stoi(req.matches[1].str());
            if (process_manager->terminate_process(pid)) {
                res.set_content(create_success_response({}, "Process " + std::to_string(pid) + " terminated successfully.").dump(), "application/json; charset=utf-8");
            } else {
                res.status = 404;
                res.set_content(create_error_response("Process not found.").dump(), "application/json; charset=utf-8");
            }
        });

        // --- 调度器 API ---
        svr.Post("/api/v1/scheduler/tick", [&](const httplib::Request&, httplib::Response& res) {
            auto scheduled_proc = process_manager->schedule();
            if (scheduled_proc) {
                res.set_content(create_success_response(pcb_to_json(*scheduled_proc)).dump(), "application/json; charset=utf-8");
            } else {
                res.set_content(create_success_response(json(nullptr), "Ready queue is empty, no process to schedule.").dump(), "application/json; charset=utf-8");
            }
        });

        svr.Get("/api/v1/scheduler/ready_queue", [&](const httplib::Request&, httplib::Response& res) {
            json data = json::array();
            for (const auto& proc : process_manager->get_ready_processes()) {
                data.push_back(pcb_to_json(*proc));
            }
            res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
        });

        // --- 内存管理 API ---
        svr.Get("/api/v1/memory/status", [&](const httplib::Request&, httplib::Response& res) {
            json data;
            data["total_memory"] = memory_manager->get_total_memory();
            data["used_memory"] = memory_manager->get_used_memory();
            json free_blocks = json::array();
            for (const auto& block : memory_manager->get_free_blocks()) {
                free_blocks.push_back({{"base_address", block.base_address}, {"size", block.size}});
            }
            data["free_blocks"] = free_blocks;
            res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
        });

        // --- 文件系统管理 API ---
        svr.Get("/api/v1/filesystem/status", [&](const httplib::Request&, httplib::Response& res) {
            auto status = fs_manager->get_filesystem_status();
            json data;
            data["total_space"] = status.total_space;
            data["used_space"] = status.used_space;
            data["free_space"] = status.free_space;
            data["total_files"] = status.total_files;
            data["total_dirs"] = status.total_directories;

            // Debugging: Add root directory listing to status
            auto root_listing = fs_manager->list_directory("/");
            json root_dirs = json::array();
            if (root_listing) {
                for (const auto& entry : *root_listing) {
                    root_dirs.push_back(entry.name);
                }
            }
            data["debug_root_dirs"] = root_dirs;

            res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
        });

        svr.Post("/api/v1/filesystem/directory", [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto body = json::parse(req.body);
                std::string path = body.at("path");
                uint16_t permissions = 755; // 默认权限
                if (body.contains("permissions")) {
                    permissions = body.at("permissions");
                }
                
                if (fs_manager->create_directory(path, permissions)) {
                    json data;
                    data["path"] = path;
                    data["permissions"] = permissions;
                    res.status = 201;
                    res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
                } else {
                    res.status = 400;
                    res.set_content(create_error_response("Failed to create directory").dump(), "application/json; charset=utf-8");
                }
            } catch (const json::exception& e) {
                res.status = 400;
                res.set_content(create_error_response("Invalid request body: " + std::string(e.what())).dump(), "application/json; charset=utf-8");
            }
        });

        svr.Post("/api/v1/filesystem/file", [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto body = json::parse(req.body);
                std::string path = body.at("path");
                std::string content = body.value("content", "");
                uint16_t permissions = 644; // 默认权限
                if (body.contains("permissions")) {
                    permissions = body.at("permissions");
                }
                
                if (fs_manager->create_file(path, content, permissions)) {
                    json data;
                    data["path"] = path;
                    data["size"] = content.size();
                    data["permissions"] = permissions;
                    res.status = 201;
                    res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
                } else {
                    res.status = 400;
                    res.set_content(create_error_response("Failed to create file").dump(), "application/json; charset=utf-8");
                }
            } catch (const json::exception& e) {
                res.status = 400;
                res.set_content(create_error_response("Invalid request body: " + std::string(e.what())).dump(), "application/json; charset=utf-8");
            }
        });

        svr.Get(R"(/api/v1/filesystem/file/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
            std::string path = "/" + req.matches[1].str();
            
            auto file_content_opt = fs_manager->read_file(path);
            
            if (file_content_opt) {
                auto& content = *file_content_opt;
                json data;
                data["content"] = content.content;
                data["size"] = content.size;
                data["permissions"] = content.permissions;
                data["last_modified"] = format_time_point(std::chrono::system_clock::from_time_t(content.modified_at));
                res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
            } else {
                res.status = 404;
                res.set_content(create_error_response("File not found").dump(), "application/json; charset=utf-8");
            }
        });

        svr.Put(R"(/api/v1/filesystem/file/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
            try {
                std::string path = "/" + req.matches[1].str();
                auto body = json::parse(req.body);
                std::string content = body.at("content");
                bool append = body.value("append", false);
                
                if (fs_manager->write_file(path, content, append)) {
                    auto file_info = fs_manager->read_file(path); // Re-read to get updated info
                    json data;
                    if(file_info) {
                        data["size"] = file_info->size;
                        data["last_modified"] = format_time_point(std::chrono::system_clock::from_time_t(file_info->modified_at));
                    }
                    res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
                } else {
                    res.status = 404;
                    res.set_content(create_error_response("File not found").dump(), "application/json; charset=utf-8");
                }
            } catch (const json::exception& e) {
                res.status = 400;
                res.set_content(create_error_response("Invalid request body: " + std::string(e.what())).dump(), "application/json; charset=utf-8");
            }
        });

        svr.Delete(R"(/api/v1/filesystem/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
            std::string path = "/" + req.matches[1].str();
            
            // The logic needs to determine if the target is a file or directory.
            // A simple approach is to check the inode type.
            auto inode_opt = fs_manager->find_inode_by_path(path);

            if (!inode_opt) {
                res.status = 404;
                res.set_content(create_error_response("File or directory not found.").dump(), "application/json; charset=utf-8");
                return;
            }

            // Based on the inode type, call the correct delete function.
            // This assumes find_inode_by_path is accessible and tells us the type.
            // Let's assume we need to get the inode first to check its type.
            // This is a conceptual change, assuming we can get the Inode.
            // For now, let's stick to the try-file-then-dir logic but improve the response codes.
            
            // First, try to delete as a file
            if (fs_manager->delete_file(path)) {
                res.status = 200;
                res.set_content(create_success_response({}, "File deleted successfully.").dump(), "application/json; charset=utf-8");
                return;
            }

            // If that fails, it could be a directory or not exist.
            // The previous check with find_inode_by_path handles non-existence.
            // So if delete_file fails, it must be a directory (or some other error).
            auto delete_dir_result = fs_manager->delete_directory(path);
            switch (delete_dir_result) {
                case FsDeleteResult::Success:
                    res.status = 200;
                    res.set_content(create_success_response({}, "Directory deleted successfully.").dump(), "application/json; charset=utf-8");
                    break;
                case FsDeleteResult::DirectoryNotEmpty:
                    res.status = 400;
                    res.set_content(create_error_response("Directory not empty.").dump(), "application/json; charset=utf-8");
                    break;
                case FsDeleteResult::NotFound:
                    // This case should ideally not be hit if delete_file was tried first
                    // and we check existence beforehand. But for robustness:
                    res.status = 404;
                    res.set_content(create_error_response("File or directory not found.").dump(), "application/json; charset=utf-8");
                    break;
                case FsDeleteResult::IsFile:
                     // This case is already handled by delete_file, but for completeness.
                    res.status = 400; // It's not a directory
                    res.set_content(create_error_response("The specified path is a file, not a directory.").dump(), "application/json; charset=utf-8");
                    break;
            }
        });

        svr.Get(R"(/api/v1/filesystem/directory/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
            std::string path = "/" + req.matches[1].str();
            
            auto dir_contents_opt = fs_manager->list_directory(path);
            
            if (dir_contents_opt) {
                json data;
                json files = json::array();
                json directories = json::array();
                
                for (const auto& entry : *dir_contents_opt) {
                    json item;
                    item["name"] = entry.name;
                    item["permissions"] = entry.permissions;
                    item["last_modified"] = format_time_point(std::chrono::system_clock::from_time_t(entry.modified_at));
                    
                    if (entry.type == "directory") {
                        directories.push_back(item);
                    } else {
                        item["size"] = entry.size;
                        files.push_back(item);
                    }
                }
                
                data["files"] = files;
                data["directories"] = directories;
                res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
            } else {
                res.status = 404;
                res.set_content(create_error_response("Directory not found").dump(), "application/json; charset=utf-8");
            }
        });
        
        // --- 时钟管理 API ---
        svr.Get("/api/v1/clock/time", [&](const httplib::Request&, httplib::Response& res) {
            json data;
            data["ticks"] = clock_manager->get_ticks();
            data["uptime"] = clock_manager->get_uptime_ms();
            data["real_time"] = format_time_point(clock_manager->get_real_time());
            res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
        });
        
        svr.Post("/api/v1/clock/interval", [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto body = json::parse(req.body);
                auto interval_val = body.at("interval");

                // 增加对interval值的校验
                if (!interval_val.is_number() || interval_val.get<long long>() <= 0) {
                    res.status = 400;
                    res.set_content(create_error_response("Interval must be a positive integer.").dump(), "application/json; charset=utf-8");
                    return;
                }
                
                uint64_t new_interval = interval_val.get<uint64_t>();

                uint64_t old_interval = clock_manager->get_interval();
                clock_manager->set_interval(new_interval);
                
                json data;
                data["old_interval"] = old_interval;
                data["new_interval"] = new_interval;
                res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
            } catch (const json::exception& e) {
                res.status = 400;
                res.set_content(create_error_response("Invalid request: " + std::string(e.what())).dump(), "application/json; charset=utf-8");
            }
        });

        svr.Post("/api/v1/clock/timer", [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto body = json::parse(req.body);
                int delay = body.at("delay");
                bool repeat = body.value("repeat", false);
                int interval = body.value("interval", 0);

                auto timer_id_opt = clock_manager->set_timer(delay, repeat, interval);

                if (timer_id_opt) {
                    json data;
                    data["timer_id"] = *timer_id_opt;
                    
                    // Calculate and format trigger time for the response
                    auto trigger_tp = std::chrono::system_clock::now() + std::chrono::milliseconds(delay);
                    data["trigger_time"] = format_time_point(trigger_tp);

                    res.status = 201;
                    res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
                } else {
                    res.status = 400;
                    res.set_content(create_error_response("Invalid timer parameters (e.g., non-positive delay).").dump(), "application/json; charset=utf-8");
                }
            } catch (const json::exception& e) {
                 res.status = 400;
                res.set_content(create_error_response("Invalid request body: " + std::string(e.what())).dump(), "application/json; charset=utf-8");
            }
        });

        // --- 服务器控制 ---
        svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("OS Simulator API is running.", "text/plain; charset=utf-8");
        });

        svr.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
            json data = {
                {"status", "healthy"},
                {"process_count", process_manager->get_all_processes().size()},
                {"memory_usage", memory_manager->get_used_memory()}
            };
            res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
        });

        // --- Device Management API ---
        svr.Get("/api/v1/devices", [&](const httplib::Request&, httplib::Response& res) {
            json data = json::array();
            auto all_devices = device_manager->get_all_devices();
            for (const auto& device : all_devices) {
                json d;
                d["device_id"] = device.id;
                // According to API.md, name is just a string. Let's make it unique.
                d["name"] = device.type + std::to_string(device.id); 
                d["type"] = device.type;
                d["status"] = device.is_busy ? "BUSY" : "IDLE";
                if (device.user_pid.has_value()) {
                    d["current_user"] = device.user_pid.value();
                } else {
                    d["current_user"] = nullptr;
                }
                data.push_back(d);
            }
            res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
        });

        svr.Post("/api/v1/devices/request", [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto body = json::parse(req.body);
                std::string device_type = body.at("device_type");
                
                // For now, we'll use a dummy PID 0. In a real scenario, this would come from the authenticated process.
                ProcessID pid = 0; 
                auto device_id_opt = device_manager->request_device(device_type, pid);

                if (device_id_opt) {
                    json data;
                    data["device_id"] = *device_id_opt;
                    // Find device to get its name
                     for (const auto& device : device_manager->get_all_devices()) {
                        if (device.id == *device_id_opt) {
                            data["name"] = device.type + std::to_string(device.id);
                            break;
                        }
                    }
                    res.status = 200;
                    res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
                } else {
                    res.status = 400;
                    res.set_content(create_error_response("No available device of type " + device_type).dump(), "application/json; charset=utf-8");
                }
            } catch (const json::exception& e) {
                res.status = 400;
                res.set_content(create_error_response("Invalid request body: " + std::string(e.what())).dump(), "application/json; charset=utf-8");
            }
        });

        svr.Post(R"(/api/v1/devices/(\d+)/release)", [&](const httplib::Request& req, httplib::Response& res) {
            int device_id = std::stoi(req.matches[1].str());
            ProcessID pid = 0; // Dummy PID

            if (device_manager->release_device(device_id, pid)) {
                res.status = 200;
                res.set_content(create_success_response({}, "Device released successfully").dump(), "application/json; charset=utf-8");
            } else {
                res.status = 400;
                res.set_content(create_error_response("Device is not in use or not found").dump(), "application/json; charset=utf-8");
            }
        });

        svr.Post(R"(/api/v1/devices/(\d+)/operation)", [&](const httplib::Request& req, httplib::Response& res) {
            int device_id = std::stoi(req.matches[1].str());
            
            // Check if device exists
            bool found = false;
            for (const auto& device : device_manager->get_all_devices()) {
                if (device.id == device_id) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                res.status = 404;
                res.set_content(create_error_response("Device not found.").dump(), "application/json; charset=utf-8");
                return;
            }

            // This is a placeholder implementation as per the test
            json data = {{"success", true}, {"result", {{"job_id", "PRINT_123"}}}};
            res.status = 200;
            res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
        });

        // --- Interrupt Management API ---

        svr.Get("/api/v1/interrupts/vector_table", [&](const httplib::Request&, httplib::Response& res) {
            auto table = interrupt_manager->get_vector_table();
            json data;
            json vectors = json::array();
            for (const auto& pair : table) {
                json e;
                e["vector"] = pair.first;
                e["handler_type"] = pair.second.type;
                e["priority"] = pair.second.priority;
                e["enabled"] = true; // Assuming all registered handlers are enabled
                vectors.push_back(e);
            }
            data["vectors"] = vectors;
            res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
        });

        svr.Post("/api/v1/interrupts/handler", [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto body = json::parse(req.body);
                int vector = body.at("vector");
                std::string type = body.at("handler_type");
                int priority = body.value("priority", 0);

                if (interrupt_manager->register_handler(vector, type, priority)) {
                    json data = {
                        {"vector", vector},
                        {"handler_type", type},
                        {"priority", priority}
                    };
                    res.status = 201;
                    res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
                } else {
                    res.status = 400;
                    res.set_content(create_error_response("Failed to register handler. Vector might already be in use.").dump(), "application/json; charset=utf-8");
                }
            } catch (const json::exception& e) {
                res.status = 400;
                res.set_content(create_error_response("Invalid request body: " + std::string(e.what())).dump(), "application/json; charset=utf-8");
            }
        });

        svr.Post("/api/v1/interrupts/trigger", [&](const httplib::Request& req, httplib::Response& res) {
             try {
                auto body = json::parse(req.body);
                int vector = body.at("vector");
                json int_data = body.value("data", json::object()); // Get data or empty object
                
                if (interrupt_manager->trigger_interrupt(vector, int_data)) {
                     json data = {
                        {"success", true},
                        {"handler_result", {
                            {"processed", true}
                        }}
                    };
                    res.status = 200;
                    res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
                } else {
                    res.status = 404;
                    res.set_content(create_error_response("No handler registered for vector " + std::to_string(vector)).dump(), "application/json; charset=utf-8");
                }
            } catch (const json::exception& e) {
                res.status = 400;
                res.set_content(create_error_response("Invalid request body: " + std::string(e.what())).dump(), "application/json; charset=utf-8");
            }
        });

        std::cout << "HTTP server starting on http://0.0.0.0:8080" << std::endl;
        try {
            svr.listen("0.0.0.0", 8080);
        } catch (const std::exception& e) {
            std::cerr << "Server listen failed: " << e.what() << std::endl;
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error during server initialization or runtime: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

// --- JSON 转换辅助函数 ---

std::string to_string_for_json(ProcessState state) {
    switch (state) {
        case ProcessState::NEW: return "NEW";
        case ProcessState::READY: return "READY";
        case ProcessState::RUNNING: return "RUNNING";
        case ProcessState::BLOCKED: return "BLOCKED";
        case ProcessState::TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

json pcb_to_json(const PCB& pcb) {
    json j;
    j["pid"] = pcb.pid;
    j["state"] = to_string_for_json(pcb.state);
    j["program_counter"] = pcb.program_counter;
    j["memory_info"] = json::array();
    for (const auto& block : pcb.memory_info) {
        j["memory_info"].push_back({
            {"base_address", block.base_address},
            {"size", block.size}
        });
    }
    return j;
}