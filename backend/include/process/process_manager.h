#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "pcb.h"
#include "../memory/memory_manager.h"
#include <vector>
#include <list>
#include <map>
#include <memory>

class ProcessManager {
public:
    ProcessManager(MemoryManager& mem_manager);

    // 创建一个新进程
    PCB* create_process(uint64_t size);

    // 终止一个进程
    bool terminate_process(pid_t pid);

    // 调度 (简单示例)
    PCB* schedule();

    // for UI
    const std::map<pid_t, std::unique_ptr<PCB>>& get_all_processes() const;
    const std::list<PCB*>& get_ready_queue() const;

private:
    MemoryManager& memory_manager;
    std::map<pid_t, std::unique_ptr<PCB>> process_table;
    std::list<PCB*> ready_queue;
    pid_t next_pid;
};

#endif //PROCESS_MANAGER_H 