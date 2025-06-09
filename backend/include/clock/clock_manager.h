#ifndef CLOCK_MANAGER_H
#define CLOCK_MANAGER_H

#include <chrono>
#include <cstdint>
#include <vector>
#include <optional>

class ClockManager {
public:
    ClockManager();

    void start();

    // 获取系统启动以来的时钟滴答数
    uint64_t get_ticks() const;

    // 获取系统启动以来的毫秒数
    uint64_t get_uptime_ms() const;

    // 获取当前真实世界时间
    std::chrono::system_clock::time_point get_real_time() const;

    // 设置时钟中断间隔 (暂未实现)
    void set_interval(uint64_t ms);
    uint64_t get_interval() const;

    // 设置定时器
    std::optional<int> set_timer(int delay_ms, bool repeat, int interval_ms);

private:
    struct Timer {
        int id;
        std::chrono::steady_clock::time_point trigger_time;
        bool repeat;
        int interval_ms;
    };

    // 模拟时钟启动的时间点
    std::chrono::steady_clock::time_point start_time_;
    // 模拟时钟中断间隔
    uint64_t interval_ms_;
    // 定时器列表
    std::vector<Timer> timers_;
    int next_timer_id_;
};

#endif // CLOCK_MANAGER_H 