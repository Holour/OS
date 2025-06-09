#pragma once

#include "../common.h"
#include "../process/process_manager.h"
#include <chrono>
#include <vector>
#include <map>
#include <optional>
#include <functional>
#include <string>
#include <nlohmann/json.hpp>

struct InterruptHandler {
    std::string type;
    int priority;
    std::function<void(const nlohmann::json&)> callback;
};

class InterruptManager {
public:
    // InterruptManager needs to interact with the process manager to change process states
    InterruptManager(ProcessManager& pm);

    // Raise an interrupt
    bool raise_interrupt(InterruptType type, ProcessID pid = -1);

    // Process all pending interrupts in the queue
    void handle_interrupts();

    // 新增API声明
    void set_clock_interval(int ms);
    std::pair<uint64_t, uint64_t> get_system_time() const;
    std::optional<int> set_timer(int duration_ms, const std::string& callback_type);
    bool register_handler(int vector, const std::string& handler_type, int priority);
    bool trigger_interrupt(int vector, const nlohmann::json& data);
    std::vector<std::pair<int, InterruptHandler>> get_vector_table();

private:
    ProcessManager& process_manager;
    int clock_interval_ms;
    std::chrono::steady_clock::time_point start_time;
    uint64_t tick_count;
    std::map<int, std::chrono::steady_clock::time_point> active_timers;
    std::map<int, InterruptHandler> handlers;
}; 