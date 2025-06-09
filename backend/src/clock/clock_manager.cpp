#include "../../include/clock/clock_manager.h"

ClockManager::ClockManager() : interval_ms_(10), next_timer_id_(1) { // 默认10ms一个时钟滴答
    start();
}

void ClockManager::start() {
    start_time_ = std::chrono::steady_clock::now();
}

uint64_t ClockManager::get_ticks() const {
    return get_uptime_ms() / interval_ms_;
}

uint64_t ClockManager::get_uptime_ms() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_).count();
}

std::chrono::system_clock::time_point ClockManager::get_real_time() const {
    return std::chrono::system_clock::now();
}

void ClockManager::set_interval(uint64_t ms) {
    if (ms > 0) {
        interval_ms_ = ms;
    }
}

uint64_t ClockManager::get_interval() const {
    return interval_ms_;
}

std::optional<int> ClockManager::set_timer(int delay_ms, bool repeat, int interval_ms) {
    if (delay_ms <= 0) {
        return std::nullopt; // Invalid delay
    }
    if (repeat && interval_ms <= 0) {
        return std::nullopt; // Invalid interval for repeating timer
    }
    
    Timer new_timer;
    new_timer.id = next_timer_id_++;
    new_timer.trigger_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
    new_timer.repeat = repeat;
    new_timer.interval_ms = interval_ms;
    
    timers_.push_back(new_timer);
    
    return new_timer.id;
} 