#include "../../include/process/process_manager.h"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <deque>

ProcessManager::ProcessManager(MemoryManager& mem_manager)
    : memory_manager(mem_manager), next_pid(1), current_running_process(nullptr) {}

void ProcessManager::set_algorithm(SchedulingAlgorithm algo, uint64_t time_slice) {
    algorithm_ = algo;
    if (algo == SchedulingAlgorithm::RR && time_slice > 0) {
        time_slice_ = time_slice;
    }
}

SchedulingAlgorithm ProcessManager::get_algorithm() const {
    return algorithm_;
}

uint64_t ProcessManager::get_time_slice() const {
    return time_slice_;
}

std::optional<ProcessID> ProcessManager::create_process(uint64_t size, uint64_t cpu_time, uint32_t priority) {
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
    
    pcb->cpu_time = cpu_time;
    pcb->remaining_time = cpu_time;
    pcb->priority = priority;
    pcb->creation_time = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());

    // 对于分区(PARTITIONED)或分页(PAGED)策略, 需要重新获取并覆盖首地址;
    // 连续分配(CONTINUOUS)策略下, allocate_for_process 已经返回了正确的 base_address, 无需覆盖
    auto current_strategy = memory_manager.get_allocation_strategy();
    if (current_strategy != MemoryAllocationStrategy::CONTINUOUS) {
        uint64_t base_address = memory_manager.get_process_base_address(new_pid);
        if (base_address != UINT64_MAX) {
            pcb->memory_info[0].base_address = base_address;
        }
    }

    all_processes[pcb->pid] = pcb;
    ready_queue.push_back(pcb);

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
    std::deque<std::shared_ptr<PCB>> new_ready_queue;
    for (const auto& p : ready_queue) {
        if (p->pid != pid) new_ready_queue.push_back(p);
    }
    ready_queue.swap(new_ready_queue);

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
    // 将当前运行进程重新放回就绪队列（RR/FCFS需要）
    if (current_running_process) {
        current_running_process->state = ProcessState::READY;
        ready_queue.push_back(current_running_process);
        current_running_process = nullptr;
    }

    if (ready_queue.empty()) {
        return nullptr;
    }

    size_t idx = 0; // 选中索引
    switch (algorithm_) {
        case SchedulingAlgorithm::FCFS:
            idx = 0; // 队首
            break;
        case SchedulingAlgorithm::SJF: {
            uint64_t min_time = UINT64_MAX;
            for (size_t i = 0; i < ready_queue.size(); ++i) {
                if (ready_queue[i]->remaining_time < min_time) {
                    min_time = ready_queue[i]->remaining_time;
                    idx = i;
                }
            }
            break;
        }
        case SchedulingAlgorithm::PRIORITY: {
            uint32_t min_pri = UINT32_MAX;
            for (size_t i = 0; i < ready_queue.size(); ++i) {
                if (ready_queue[i]->priority < min_pri) {
                    min_pri = ready_queue[i]->priority;
                    idx = i;
                }
            }
            break; }
        case SchedulingAlgorithm::RR:
            idx = 0;
            break;
    }

    current_running_process = ready_queue[idx];
    ready_queue.erase(ready_queue.begin() + idx);
    current_running_process->state = ProcessState::RUNNING;

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
    bool found = false;
    std::shared_ptr<PCB> pcb_to_block = nullptr;
    std::deque<std::shared_ptr<PCB>> new_ready_queue;
    for (const auto& p : ready_queue) {
        if (p->pid == pid) {
            found = true;
            pcb_to_block = p;
        } else {
            new_ready_queue.push_back(p);
        }
    }
    ready_queue.swap(new_ready_queue);

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
        ready_queue.push_back(pcb);
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
    std::vector<std::shared_ptr<PCB>> ready_vec(ready_queue.begin(), ready_queue.end());
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

// ------------------ 甘特图生成 -------------------
std::vector<ProcessManager::GanttEntry> ProcessManager::generate_gantt_chart() const {
    std::vector<GanttEntry> table;

    if (all_processes.empty()) return table;

    // 收集所有进程（包含 TERMINATED 之外的所有状态）
    struct SimProc {
        ProcessID pid;
        uint64_t cpu_time;   // 进程总 CPU 时间
        uint32_t priority;
        uint64_t creation_time;
    };

    std::vector<SimProc> procs;
    procs.reserve(all_processes.size());
    for (const auto& [pid, pcb_ptr] : all_processes) {
        // 如果进程 cpu_time == 0 则跳过
        if (!pcb_ptr || pcb_ptr->cpu_time == 0) continue;
        procs.push_back({pid, pcb_ptr->cpu_time, pcb_ptr->priority, pcb_ptr->creation_time});
    }

    if (procs.empty()) return table;

    uint64_t current_time = 0;

    switch (algorithm_) {
        case SchedulingAlgorithm::FCFS: {
            std::sort(procs.begin(), procs.end(), [](const SimProc& a, const SimProc& b){ return a.creation_time < b.creation_time; });
            for (const auto& p : procs) {
                table.push_back({p.pid, current_time, current_time + p.cpu_time});
                current_time += p.cpu_time;
            }
            break;
        }
        case SchedulingAlgorithm::SJF: {
            std::sort(procs.begin(), procs.end(), [](const SimProc& a, const SimProc& b){ return a.cpu_time < b.cpu_time; });
            for (const auto& p : procs) {
                table.push_back({p.pid, current_time, current_time + p.cpu_time});
                current_time += p.cpu_time;
            }
            break;
        }
        case SchedulingAlgorithm::PRIORITY: {
            std::sort(procs.begin(), procs.end(), [](const SimProc& a, const SimProc& b){ return a.priority < b.priority; });
            for (const auto& p : procs) {
                table.push_back({p.pid, current_time, current_time + p.cpu_time});
                current_time += p.cpu_time;
            }
            break;
        }
        case SchedulingAlgorithm::RR: {
            uint64_t ts = time_slice_ > 0 ? time_slice_ : 1;

            // 使用 deque 进行轮转调度模拟，存储 <pid, remaining_time>
            std::deque<std::pair<ProcessID, uint64_t>> q;
            for (const auto& p : procs) {
                q.emplace_back(p.pid, p.cpu_time);
            }

            while (!q.empty()) {
                auto [pid, remain] = q.front();
                q.pop_front();

                uint64_t exec = std::min<uint64_t>(ts, remain);
                // 避免生成长度为 0 的片段
                if (exec == 0) continue;

                table.push_back({pid, current_time, current_time + exec});
                current_time += exec;
                remain -= exec;
                if (remain > 0) {
                    q.emplace_back(pid, remain);
                }
            }
            break;
        }
    }

    return table;
} 