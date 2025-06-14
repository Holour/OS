#pragma once

#include "pcb.h"
#include "../memory/memory_manager.h"
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <deque>
#include <optional>
#include <queue>
#include "../common.h"

class ProcessManager {
public:
    explicit ProcessManager(MemoryManager& mem_manager);

    std::optional<ProcessID> create_process(uint64_t size, uint64_t cpu_time, uint32_t priority);
    // 旧接口兼容
    std::optional<ProcessID> create_process(uint64_t size) { return create_process(size, 10, 5); }
    bool terminate_process(ProcessID pid);
    
    // Scheduling
    std::shared_ptr<PCB> schedule();
    // 兼容旧测试接口
    std::shared_ptr<PCB> tick_schedule() { return schedule(); }
    
    // For interrupts
    bool block_process(ProcessID pid);
    bool wakeup_process(ProcessID pid);

    // For UI/API
    std::shared_ptr<PCB> get_running_process() const;
    std::vector<std::shared_ptr<PCB>> get_ready_processes() const;
    const std::deque<std::shared_ptr<PCB>>& get_ready_queue() const { return ready_queue; }
    std::vector<std::shared_ptr<PCB>> get_blocked_processes() const;
    std::shared_ptr<PCB> get_process(ProcessID pid) const;
    std::vector<std::shared_ptr<PCB>> get_all_processes() const;
    
    // 调度相关 API
    void set_algorithm(SchedulingAlgorithm algo, uint64_t time_slice = 1);
    SchedulingAlgorithm get_algorithm() const;
    uint64_t get_time_slice() const;

    // 生成甘特图数据（简单模拟）：返回 {pid,start,end}
    struct GanttEntry { ProcessID pid; uint64_t start; uint64_t end; };
    std::vector<GanttEntry> generate_gantt_chart() const;

private:
    MemoryManager& memory_manager;
    ProcessID next_pid;
    
    std::map<ProcessID, std::shared_ptr<PCB>> all_processes;
    std::deque<std::shared_ptr<PCB>> ready_queue;
    std::list<std::shared_ptr<PCB>> blocked_processes;
    std::shared_ptr<PCB> current_running_process;

    // 调度算法
    SchedulingAlgorithm algorithm_ = SchedulingAlgorithm::FCFS;
    uint64_t time_slice_ = 1;
}; 