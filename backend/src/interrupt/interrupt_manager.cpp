#include "../include/interrupt/interrupt_manager.h"
#include <iostream>
#include <algorithm>

InterruptManager::InterruptManager(ProcessManager& pm)
    : process_manager(pm), clock_interval_ms(1000), start_time(std::chrono::steady_clock::now()), tick_count(0) {
    std::cout << "InterruptManager initialized" << std::endl;
}

void InterruptManager::set_clock_interval(int ms) {
    if (ms <= 0) {
        std::cerr << "Invalid clock interval: " << ms << std::endl;
        return;
    }
    clock_interval_ms = ms;
}

std::pair<uint64_t, uint64_t> InterruptManager::get_system_time() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
    return {elapsed.count(), tick_count};
}

std::optional<int> InterruptManager::set_timer(int duration_ms, const std::string& callback_type) {
    if (duration_ms <= 0) {
        std::cerr << "Invalid timer duration: " << duration_ms << std::endl;
        return std::nullopt;
    }
    static int next_timer_id = 1;
    int timer_id = next_timer_id++;
    auto expiration = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);
    active_timers[timer_id] = expiration;
    return timer_id;
}

bool InterruptManager::register_handler(int vector, const std::string& handler_type, int priority) {
    if (handlers.count(vector)) {
        return false; // Vector already has a registered handler.
    }
    if (vector < 0) {
        std::cerr << "Invalid interrupt vector: " << vector << std::endl;
        return false;
    }
    InterruptHandler handler;
    handler.type = handler_type;
    handler.priority = priority;
    handler.callback = [this](const nlohmann::json& data) {
        // 默认回调实现，例如唤醒进程
        if (data.contains("pid")) {
            ProcessID pid = data["pid"];
            process_manager.wakeup_process(pid);
        }
    };
    handlers[vector] = handler;
    return true;
}

bool InterruptManager::trigger_interrupt(int vector, const nlohmann::json& data) {
    auto it = handlers.find(vector);
    if (it == handlers.end()) {
        std::cerr << "No handler registered for vector: " << vector << std::endl;
        return false;
    }
    it->second.callback(data);
    return true;
}

std::vector<std::pair<int, InterruptHandler>> InterruptManager::get_vector_table() {
    std::vector<std::pair<int, InterruptHandler>> table;
    for (const auto& [vector, handler] : handlers) {
        table.emplace_back(vector, handler);
    }
    std::sort(table.begin(), table.end(), [](const auto& a, const auto& b) {
        return a.second.priority > b.second.priority;
    });
    return table;
}

bool InterruptManager::raise_interrupt(InterruptType type, ProcessID pid) {
    switch (type) {
        case InterruptType::TIMER: {
            tick_count++;
            auto now = std::chrono::steady_clock::now();
            for (auto it = active_timers.begin(); it != active_timers.end();) {
                if (now >= it->second) {
                    trigger_interrupt(it->first, nlohmann::json{{"timer_id", it->first}});
                    it = active_timers.erase(it);
                } else {
                    ++it;
                }
            }
            break;
        }
        case InterruptType::DEVICE_IO:
            // 处理设备I/O中断
            break;
        default:
            std::cerr << "Unhandled interrupt type" << std::endl;
            return false;
    }
    return true;
}

void InterruptManager::handle_interrupts() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
    if (elapsed.count() >= clock_interval_ms) {
        raise_interrupt(InterruptType::TIMER);
        start_time = now;
    }
} 