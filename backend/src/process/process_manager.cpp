#include "process/process_manager.h"
#include <iostream>

ProcessManager::ProcessManager(MemoryManager& mem_manager)
    : memory_manager(mem_manager), next_pid(1) {}

PCB* ProcessManager::create_process(uint64_t size) {
    // 1. 请求内存
    auto mem_addr = memory_manager.allocate(size);

    // 2. 处理内存不足的情况
    if (!mem_addr.has_value()) {
        std::cerr << "Error: Not enough memory to create process of size " << size << std::endl;
        return nullptr;
    }

    // 3. 创建 PCB
    pid_t new_pid = next_pid++;
    auto pcb = std::make_unique<PCB>(new_pid);
    pcb->state = ProcessState::READY;
    pcb->memory_info.push_back({mem_addr.value(), size});
    
    // 4. 加入就绪队列和进程表
    ready_queue.push_back(pcb.get());
    PCB* pcb_ptr = pcb.get();
    process_table[new_pid] = std::move(pcb);

    std::cout << "Process " << new_pid << " created. Size: " << size << std::endl;
    return pcb_ptr;
}

bool ProcessManager::terminate_process(pid_t pid) {
    auto it = process_table.find(pid);
    if (it == process_table.end()) {
        return false; // 进程不存在
    }

    // 释放内存
    for (const auto& block : it->second->memory_info) {
        memory_manager.free(block.base_address, block.size);
    }

    // 从就绪队列中移除
    ready_queue.remove(it->second.get());

    // 从进程表中移除
    process_table.erase(it);

    std::cout << "Process " << pid << " terminated." << std::endl;
    return true;
}

PCB* ProcessManager::schedule() {
    if (ready_queue.empty()) {
        return nullptr; // 没有就绪进程
    }
    // 最简单的FIFO调度
    PCB* next_process = ready_queue.front();
    ready_queue.pop_front();
    
    // 模拟执行，重新放回队尾 (类似Round-Robin)
    next_process->state = ProcessState::RUNNING;
    // ... 模拟执行后 ...
    // next_process->state = ProcessState::READY;
    // ready_queue.push_back(next_process);

    return next_process;
}

const std::map<pid_t, std::unique_ptr<PCB>>& ProcessManager::get_all_processes() const {
    return process_table;
}

const std::list<PCB*>& ProcessManager::get_ready_queue() const {
    return ready_queue;
} 