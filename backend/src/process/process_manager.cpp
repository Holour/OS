#include "../../include/process/process_manager.h"
#include <algorithm>
#include <iostream>

ProcessManager::ProcessManager(MemoryManager& mem_manager)
    : memory_manager(mem_manager), next_pid(1), current_running_process(nullptr) {}

std::optional<ProcessID> ProcessManager::create_process(uint64_t size) {
    if (size == 0) {
        return std::nullopt;
    }

    ProcessID new_pid = next_pid++;
    auto block_opt = memory_manager.allocate_for_process(new_pid, size);
    if (!block_opt) {
        next_pid--; // 回退PID
        return std::nullopt; // Not enough memory
    }

    auto pcb = std::make_shared<PCB>();
    pcb->pid = new_pid;
    pcb->state = ProcessState::READY;
    pcb->memory_info.push_back(block_opt.value());
    
    // 计算并设置进程起始地址
    uint64_t base_address = memory_manager.get_process_base_address(new_pid);
    if (base_address != UINT64_MAX) {
        pcb->memory_info[0].base_address = base_address;
    }

    all_processes[pcb->pid] = pcb;
    ready_queue.push(pcb);

    return pcb->pid;
}

bool ProcessManager::terminate_process(ProcessID pid) {
    auto it = all_processes.find(pid);
    if (it == all_processes.end()) {
        return false;
    }

    auto pcb = it->second;

    // 尝试使用新的进程内存释放方法
    bool memory_freed = memory_manager.free_process_memory(pid);
    
    // 如果新方法失败，使用传统方法（用于连续分配）
    if (!memory_freed) {
        for (const auto& block : pcb->memory_info) {
            memory_manager.free(block.base_address, block.size);
        }
    }

    // Remove from all_processes map
    all_processes.erase(it);

    // If it was the running process, set current_running_process to nullptr
    if (current_running_process && current_running_process->pid == pid) {
        current_running_process = nullptr;
    }
    
    // Remove from ready queue
    std::queue<std::shared_ptr<PCB>> new_ready_queue;
    while (!ready_queue.empty()) {
        auto p = ready_queue.front();
        ready_queue.pop();
        if (p->pid != pid) {
            new_ready_queue.push(p);
        }
    }
    ready_queue = new_ready_queue;

    // Remove from blocked queue
    std::list<std::shared_ptr<PCB>> new_blocked_list;
    for(const auto& p : blocked_processes) {
        if (p->pid != pid) {
            new_blocked_list.push_back(p);
        }
    }
    blocked_processes = new_blocked_list;

    return true;
}

std::shared_ptr<PCB> ProcessManager::schedule() {
    // If there's a running process, move it to the back of the ready queue.
    if (current_running_process) {
        current_running_process->state = ProcessState::READY;
        ready_queue.push(current_running_process);
        current_running_process = nullptr;
    }

    // If there are ready processes, pick the next one.
    if (!ready_queue.empty()) {
        current_running_process = ready_queue.front();
        ready_queue.pop();
        current_running_process->state = ProcessState::RUNNING;
    }
    
    return current_running_process;
}

bool ProcessManager::block_process(ProcessID pid) {
    // Find in running process
    if (current_running_process && current_running_process->pid == pid) {
        auto pcb = current_running_process;
        pcb->state = ProcessState::BLOCKED;
        blocked_processes.push_back(pcb);
        current_running_process = nullptr;
        // After blocking the current process, immediately schedule the next one
        schedule();
        return true;
    }

    // Find in ready queue
    std::queue<std::shared_ptr<PCB>> new_ready_queue;
    bool found = false;
    std::shared_ptr<PCB> pcb_to_block = nullptr;
    while(!ready_queue.empty()){
        auto p = ready_queue.front();
        ready_queue.pop();
        if(p->pid == pid){
            found = true;
            pcb_to_block = p;
        } else {
            new_ready_queue.push(p);
        }
    }
    ready_queue = new_ready_queue;

    if (found) {
        pcb_to_block->state = ProcessState::BLOCKED;
        blocked_processes.push_back(pcb_to_block);
        return true;
    }

    return false; // Process not found in ready or running state
}

bool ProcessManager::wakeup_process(ProcessID pid) {
    auto it = std::find_if(blocked_processes.begin(), blocked_processes.end(), 
        [pid](const std::shared_ptr<PCB>& pcb){
            return pcb->pid == pid;
        });

    if (it != blocked_processes.end()) {
        auto pcb = *it;
        pcb->state = ProcessState::READY;
        ready_queue.push(pcb);
        blocked_processes.erase(it);
        return true;
    }

    return false; // Process not found in blocked queue
}

// --- Methods for UI/API ---

std::shared_ptr<PCB> ProcessManager::get_running_process() const {
    return current_running_process;
}

std::vector<std::shared_ptr<PCB>> ProcessManager::get_ready_processes() const {
    std::vector<std::shared_ptr<PCB>> ready_vec;
    std::queue<std::shared_ptr<PCB>> temp_q = ready_queue;
    while(!temp_q.empty()){
        ready_vec.push_back(temp_q.front());
        temp_q.pop();
    }
    return ready_vec;
}

std::vector<std::shared_ptr<PCB>> ProcessManager::get_blocked_processes() const {
    std::vector<std::shared_ptr<PCB>> blocked_vec;
    for(const auto& pcb : blocked_processes){
        blocked_vec.push_back(pcb);
    }
    return blocked_vec;
}

std::shared_ptr<PCB> ProcessManager::get_process(ProcessID pid) const {
    auto it = all_processes.find(pid);
    if (it != all_processes.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<PCB>> ProcessManager::get_all_processes() const {
    std::vector<std::shared_ptr<PCB>> processes;
    for (const auto& pair : all_processes) {
        processes.push_back(pair.second);
    }
    return processes;
} 