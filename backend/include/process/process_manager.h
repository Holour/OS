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

    std::optional<ProcessID> create_process(const std::string& name, uint64_t size, uint64_t cpu_time, uint32_t priority, ProcessID parent_pid = -1);
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

    // 创建子进程（fork 风格），继承父进程部分属性
    std::optional<ProcessID> create_child_process(ProcessID parent_pid, const std::string& child_name, uint64_t size, uint64_t cpu_time, uint32_t priority);

    // 进程状态更新
    bool update_process_state(ProcessID pid, ProcessState new_state);

    // 进程关系类型
    enum class RelationType { SYNC, MUTEX };
    bool create_process_relationship(ProcessID pid1, ProcessID pid2, RelationType type);

    // 基础接口（保持向后兼容）
    std::optional<ProcessID> create_process(uint64_t size, uint64_t cpu_time, uint32_t priority);

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

    // 进程关系映射: pid -> (另一端 pid, 关系类型)
    std::multimap<ProcessID, std::pair<ProcessID, RelationType>> relations_;
}; 