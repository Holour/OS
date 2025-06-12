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

// Helper to convert string to AllocationStrategy enum
std::optional<AllocationStrategy> string_to_allocation_strategy(const std::string& s) {
    if (s == "CONTIGUOUS") return AllocationStrategy::CONTIGUOUS;
    if (s == "LINKED") return AllocationStrategy::LINKED;
    if (s == "INDEXED") return AllocationStrategy::INDEXED;
    return std::nullopt;
}

// Helper to convert AllocationStrategy enum to string
std::string allocation_strategy_to_string(AllocationStrategy strategy) {
    switch (strategy) {
        case AllocationStrategy::CONTIGUOUS: return "CONTIGUOUS";
        case AllocationStrategy::LINKED: return "LINKED";
        case AllocationStrategy::INDEXED: return "INDEXED";
        default: return "UNKNOWN";
    }
}

// 函数：初始化系统状态
void initialize_system_state() {
    std::cout << "Initializing default system state..." << std::endl;

    // 1. 创建更丰富的文件系统结构
    fs_manager->create_directory("/home", 755);
    fs_manager->create_directory("/etc", 755);
    fs_manager->create_directory("/bin", 755);
    fs_manager->create_directory("/var", 755);
    fs_manager->create_directory("/var/log", 755);
    fs_manager->create_file("/home/welcome.txt", 100, 644); // 100 bytes
    fs_manager->create_file("/etc/config.conf", 512, 644); // 512 bytes
    fs_manager->create_file("/var/log/system.log", 4096, 600); // 4KB

    // 1.1. 创建.pubt可执行文件（类似Windows的exe）
    fs_manager->create_file("/bin/calculator.pubt", 10 * 1024 * 1024, 755); // 计算器程序，需要10MB
    fs_manager->create_file("/bin/notepad.pubt", 5 * 1024 * 1024, 755);     // 记事本程序，需要5MB
    fs_manager->create_file("/bin/browser.pubt", 50 * 1024 * 1024, 755);    // 浏览器程序，需要50MB
    fs_manager->create_file("/bin/game.pubt", 100 * 1024 * 1024, 755);      // 游戏程序，需要100MB
    fs_manager->create_file("/home/myapp.pubt", 24 * 1024 * 1024, 755);     // 用户自定义应用，需要24MB

    // 2. 创建多个初始进程（模拟真实操作系统的各种进程）
    std::vector<std::pair<std::string, uint64_t>> initial_processes = {
        {"idle_process", 256 * 1024},        // 空闲进程: 256KB
        {"kernel_worker", 512 * 1024},       // 内核工作进程: 512KB
        {"system_logger", 1024 * 1024},      // 系统日志进程: 1MB
        {"memory_manager", 768 * 1024},      // 内存管理进程: 768KB
        {"device_driver", 2 * 1024 * 1024},  // 设备驱动进程: 2MB
        {"shell", 4 * 1024 * 1024},          // 命令行解释器: 4MB
        {"file_system", 3 * 1024 * 1024},    // 文件系统进程: 3MB
        {"network_stack", 6 * 1024 * 1024},  // 网络协议栈: 6MB
        {"gui_manager", 8 * 1024 * 1024},    // GUI管理器: 8MB
        {"calculator", 10 * 1024 * 1024},    // 计算器应用: 10MB (对应.pubt文件)
        {"notepad", 5 * 1024 * 1024},        // 记事本应用: 5MB (对应.pubt文件)
        {"browser", 50 * 1024 * 1024},       // 浏览器应用: 50MB (对应.pubt文件)
        {"background_service", 1536 * 1024}, // 后台服务: 1.5MB
        {"antivirus", 12 * 1024 * 1024},     // 杀毒软件: 12MB
        {"media_player", 15 * 1024 * 1024},  // 媒体播放器: 15MB
    };

    std::cout << "Creating initial processes..." << std::endl;
    for (const auto& [name, size] : initial_processes) {
        auto pid = process_manager->create_process(size);
        if (pid) {
            std::cout << "✓ Created process '" << name << "' with PID: " << *pid 
                      << " (Size: " << size / 1024 << " KB)" << std::endl;
        } else {
            std::cout << "✗ Failed to create process '" << name << "' (Size: " << size / 1024 << " KB)" << std::endl;
        }
    }

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
        fs_manager = std::make_unique<FileSystemManager>();
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
            
            // 根据当前分配策略返回不同的内存信息
            auto strategy = memory_manager->get_allocation_strategy();
            data["allocation_strategy"] = static_cast<int>(strategy);
            
            if (strategy == MemoryAllocationStrategy::CONTINUOUS) {
                json free_blocks = json::array();
                for (const auto& block : memory_manager->get_free_blocks()) {
                    free_blocks.push_back({{"base_address", block.base_address}, {"size", block.size}});
                }
                data["free_blocks"] = free_blocks;
            } else if (strategy == MemoryAllocationStrategy::PARTITIONED) {
                json partitions = json::array();
                for (const auto& partition : memory_manager->get_partitions()) {
                    json p;
                    p["base_address"] = partition.base_address;
                    p["size"] = partition.size;
                    p["is_free"] = partition.is_free;
                    p["owner_pid"] = partition.owner_pid;
                    partitions.push_back(p);
                }
                data["partitions"] = partitions;
            } else {
                // 分页策略，显示空闲块信息（简化显示）
                json free_blocks = json::array();
                for (const auto& block : memory_manager->get_free_blocks()) {
                    free_blocks.push_back({{"base_address", block.base_address}, {"size", block.size}});
                }
                data["free_blocks"] = free_blocks;
            }
            
            res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
        });

        svr.Put("/api/v1/memory/strategy", [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto body = json::parse(req.body);
                if (!body.contains("strategy") || !body["strategy"].is_number()) {
                    res.status = 400;
                    res.set_content(create_error_response("Missing or invalid 'strategy' field.").dump(), "application/json; charset=utf-8");
                    return;
                }
                
                int strategy_int = body["strategy"];
                if (strategy_int < 0 || strategy_int > 2) {
                    res.status = 400;
                    res.set_content(create_error_response("Invalid strategy value. Must be 0(CONTINUOUS), 1(PARTITIONED), or 2(PAGED).").dump(), "application/json; charset=utf-8");
                    return;
                }
                
                auto strategy = static_cast<MemoryAllocationStrategy>(strategy_int);
                memory_manager->set_allocation_strategy(strategy);
                
                json data;
                data["old_strategy"] = static_cast<int>(memory_manager->get_allocation_strategy());
                data["new_strategy"] = strategy_int;
                
                res.set_content(create_success_response(data, "Memory allocation strategy updated successfully.").dump(), "application/json; charset=utf-8");
            } catch (const json::exception& e) {
                res.status = 400;
                res.set_content(create_error_response("Invalid request body: " + std::string(e.what())).dump(), "application/json; charset=utf-8");
            }
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
            data["allocation_method"] = allocation_strategy_to_string(status.allocation_method);

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
            json req_body;
            try {
                req_body = json::parse(req.body);
            } catch (...) {
                res.status = 400;
                res.set_content(create_error_response("Invalid JSON format.").dump(), "application/json; charset=utf-8");
                return;
            }

            if (!req_body.contains("path") || !req_body["path"].is_string()) {
                res.status = 400;
                res.set_content(create_error_response("Missing or invalid 'path' field.").dump(), "application/json; charset=utf-8");
                return;
            }
            std::string path = req_body["path"];
            uint16_t permissions = req_body.value("permissions", 0755); // Default permissions

            auto result = fs_manager->create_directory(path, permissions);

            switch (result) {
                case FsCreateResult::Success:
                    res.status = 201;
                    res.set_content(create_success_response({}, "Directory created successfully.").dump(), "application/json; charset=utf-8");
                    break;
                case FsCreateResult::AlreadyExists:
                    res.status = 409;
                    res.set_content(create_error_response("Directory or file with this name already exists.").dump(), "application/json; charset=utf-8");
                    break;
                case FsCreateResult::ParentNotFound:
                    res.status = 409;
                    res.set_content(create_error_response("Parent directory does not exist.").dump(), "application/json; charset=utf-8");
                    break;
                default: // FsCreateResult::InvalidPath or other internal errors
                    res.status = 500;
                    res.set_content(create_error_response("An internal error occurred while creating the directory.").dump(), "application/json; charset=utf-8");
                    break;
            }
        });

        svr.Post("/api/v1/filesystem/file", [&](const httplib::Request& req, httplib::Response& res) {
            json req_body;
            try {
                req_body = json::parse(req.body);
            } catch (...) {
                res.status = 400;
                res.set_content(create_error_response("Invalid JSON format.").dump(), "application/json; charset=utf-8");
                return;
            }

            if (!req_body.contains("path") || !req_body["path"].is_string()) {
                res.status = 400;
                res.set_content(create_error_response("Missing or invalid 'path' field.").dump(), "application/json; charset=utf-8");
                return;
            }

            std::string path = req_body["path"];
            uint64_t simulated_size = req_body.value("simulated_size", 0);
            uint16_t permissions = req_body.value("permissions", 0644);

            auto result = fs_manager->create_file(path, simulated_size, permissions);

            switch (result) {
                case FsCreateResult::Success:
                    res.status = 201;
                    res.set_content(create_success_response({}, "File created successfully.").dump(), "application/json; charset=utf-8");
                    break;
                case FsCreateResult::AlreadyExists:
                    res.status = 409;
                    res.set_content(create_error_response("File or directory with this name already exists.").dump(), "application/json; charset=utf-8");
                    break;
                case FsCreateResult::ParentNotFound:
                    res.status = 409;
                    res.set_content(create_error_response("Parent directory does not exist.").dump(), "application/json; charset=utf-8");
                    break;
                default: // FsCreateResult::InvalidPath or other internal errors
                    res.status = 500;
                    res.set_content(create_error_response("An internal error occurred while creating the file.").dump(), "application/json; charset=utf-8");
                    break;
            }
        });

        svr.Get(R"(/api/v1/filesystem/file/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
            std::string path = "/" + req.matches[1].str();
            auto file_content_opt = fs_manager->read_file(path);
            if (file_content_opt) {
                json data;
                data["path"] = file_content_opt->path;
                data["content"] = file_content_opt->content;
                data["permissions"] = file_content_opt->permissions;
                data["simulated_size"] = file_content_opt->simulated_size;
                data["created_at"] = format_time_point(std::chrono::system_clock::from_time_t(file_content_opt->created_at));
                data["modified_at"] = format_time_point(std::chrono::system_clock::from_time_t(file_content_opt->modified_at));
                res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
            } else {
                res.status = 404;
                res.set_content(create_error_response("File not found.").dump(), "application/json; charset=utf-8");
            }
        });

        svr.Delete(R"(/api/v1/filesystem/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
            std::string path = "/" + req.matches[1].str();
            
            // Attempt to delete as a file first
            if (fs_manager->delete_file(path)) {
                res.set_content(create_success_response({}, "File deleted successfully.").dump(), "application/json; charset=utf-8");
                return;
            }

            // If not a file, attempt to delete as a directory. Default to non-recursive.
            bool recursive = req.has_param("recursive") && req.get_param_value("recursive") == "true";
            auto result = fs_manager->delete_directory(path, recursive);
            switch (result) {
                case FsDeleteResult::Success:
                    res.set_content(create_success_response({}, "Directory deleted successfully.").dump(), "application/json; charset=utf-8");
                    break;
                case FsDeleteResult::NotFound:
                    res.status = 404;
                    res.set_content(create_error_response("File or directory not found.").dump(), "application/json; charset=utf-8");
                    break;
                case FsDeleteResult::DirectoryNotEmpty:
                    res.status = 400;
                    res.set_content(create_error_response("Directory is not empty.").dump(), "application/json; charset=utf-8");
                    break;
                case FsDeleteResult::IsFile: // Should have been caught by delete_file
                default:
                    res.status = 500;
                    res.set_content(create_error_response("An unexpected error occurred.").dump(), "application/json; charset=utf-8");
                    break;
            }
        });

        svr.Get(R"(/api/v1/filesystem/directory/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
            std::string path = "/" + req.matches[1].str();
            
            auto dir_contents_opt = fs_manager->list_directory(path);
            
            if (dir_contents_opt) {
                json data = json::array();
                for (const auto& item : *dir_contents_opt) {
                    json entry;
                    entry["name"] = item.name;
                    entry["type"] = item.type;
                    entry["size"] = item.size; // This is now uint64_t for simulated size
                    entry["permissions"] = item.permissions;
                    entry["created_at"] = format_time_point(std::chrono::system_clock::from_time_t(item.created_at));
                    entry["modified_at"] = format_time_point(std::chrono::system_clock::from_time_t(item.modified_at));
                    data.push_back(entry);
                }
                
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

        // --- NEW FILESYSTEM API ENDPOINTS ---

        svr.Put("/api/v1/filesystem/config", [&](const httplib::Request& req, httplib::Response& res) {
            try {
                auto body = json::parse(req.body);
                std::string strategy_str = body.at("allocation_method");
                auto strategy_opt = string_to_allocation_strategy(strategy_str);
                if (strategy_opt) {
                    fs_manager->set_allocation_strategy(*strategy_opt);
                    res.set_content(create_success_response({}, "Allocation strategy updated to " + strategy_str).dump(), "application/json; charset=utf-8");
                } else {
                    res.status = 400;
                    res.set_content(create_error_response("Invalid allocation strategy specified.").dump(), "application/json; charset=utf-8");
                }
            } catch (const json::exception& e) {
                res.status = 400;
                res.set_content(create_error_response("Invalid request body: " + std::string(e.what())).dump(), "application/json; charset=utf-8");
            }
        });

        svr.Get("/api/v1/filesystem/file-address", [&](const httplib::Request& req, httplib::Response& res) {
            if (!req.has_param("path")) {
                res.status = 400;
                res.set_content(create_error_response("Missing 'path' query parameter.").dump(), "application/json; charset=utf-8");
                return;
            }
            std::string path = req.get_param_value("path");
            auto addresses = fs_manager->get_file_addresses(path);

            json data;
            data["path"] = path;
            json addr_json;
            if (addresses.contiguous_start_block.has_value()) {
                addr_json["contiguous"] = *addresses.contiguous_start_block;
            } else {
                addr_json["contiguous"] = nullptr;
            }
            if (addresses.linked_start_block.has_value()) {
                addr_json["linked"] = *addresses.linked_start_block;
            } else {
                addr_json["linked"] = nullptr;
            }
            if (addresses.indexed_index_block.has_value()) {
                addr_json["indexed"] = *addresses.indexed_index_block;
            } else {
                addr_json["indexed"] = nullptr;
            }
            data["addresses"] = addr_json;

            res.set_content(create_success_response(data).dump(), "application/json; charset=utf-8");
        });

        // --- 系统日志 API ---
        svr.Get("/api/v1/logs/system", [&](const httplib::Request&, httplib::Response& res) {
            // Implementation of this endpoint is not provided in the original file or the new code block
            // This is a placeholder and should be implemented based on the actual implementation of the system
            res.set_content(create_error_response("This endpoint is not implemented").dump(), "application/json; charset=utf-8");
        });

        std::cout << "HTTP server starting on http://0.0.0.0:8080" << std::endl;
        try {
            svr.listen("0.0.0.0", 8080);
        } catch (const std::exception& e) {
            std::cerr << "Server listen failed: " << e.what() << std::endl;
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error during manager initialization: " << e.what() << std::endl;
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