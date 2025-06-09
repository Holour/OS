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

    std::optional<ProcessID> create_process(uint64_t size);
    bool terminate_process(ProcessID pid);
    
    // Scheduling
    std::shared_ptr<PCB> schedule();
    
    // For interrupts
    bool block_process(ProcessID pid);
    bool wakeup_process(ProcessID pid);

    // For UI/API
    std::shared_ptr<PCB> get_running_process() const;
    std::vector<std::shared_ptr<PCB>> get_ready_processes() const;
    std::vector<std::shared_ptr<PCB>> get_blocked_processes() const;
    std::shared_ptr<PCB> get_process(ProcessID pid) const;
    std::vector<std::shared_ptr<PCB>> get_all_processes() const;
    
private:
    MemoryManager& memory_manager;
    ProcessID next_pid;
    
    std::map<ProcessID, std::shared_ptr<PCB>> all_processes;
    std::queue<std::shared_ptr<PCB>> ready_queue;
    std::list<std::shared_ptr<PCB>> blocked_processes;
    std::shared_ptr<PCB> current_running_process;
}; 