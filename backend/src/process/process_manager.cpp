#include "../../include/process/process_manager.h"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <deque>
#include <unordered_set>
#include <functional>
#include <set>

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
    // 调用带名称版本，使用默认空名称和无父进程
    return create_process("", size, cpu_time, priority, -1);
}

std::optional<ProcessID> ProcessManager::create_process(const std::string& name, uint64_t size, uint64_t cpu_time, uint32_t priority, ProcessID parent_pid) {
    if (size == 0) {
        return std::nullopt;
    }

    ProcessID new_pid = next_pid++;
    auto block_opt = memory_manager.allocate_for_process(new_pid, size);
    if (!block_opt) {
        next_pid--; // 回退PID
        return std::nullopt; // 内存不足
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
    pcb->name = name.empty() ? ("process_" + std::to_string(new_pid)) : name;
    pcb->parent_pid = parent_pid;

    // 挂载到父进程的子进程列表（如果需要）
    if (parent_pid != -1) {
        auto parent_pcb = get_process(parent_pid);
        if (parent_pcb) {
            // 目前仅在内部记录 parent_pid 字段即可
        }
    }

    // 根据策略覆盖 base 地址
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

std::optional<ProcessID> ProcessManager::create_child_process(ProcessID parent_pid, const std::string& child_name, uint64_t size, uint64_t cpu_time, uint32_t priority) {
    // 调用带名称的新建进程接口，提供 parent_pid
    return create_process(child_name, size, cpu_time, priority, parent_pid);
}

bool ProcessManager::update_process_state(ProcessID pid, ProcessState new_state) {
    std::unordered_set<ProcessID> visited;
    std::function<void(ProcessID, ProcessState)> dfs = [&](ProcessID cur, ProcessState state){
        if (visited.count(cur)) return;
        visited.insert(cur);

        // ----- 本进程状态变更逻辑（源自旧实现） -----
        auto pcb = get_process(cur);
        if (!pcb) return;

        // 若状态未变更则跳过
        if (pcb->state == state) {
            // 但仍需传播
        } else {
            // 从旧队列移除
            switch (pcb->state) {
                case ProcessState::READY: {
                    ready_queue.erase(std::remove_if(ready_queue.begin(), ready_queue.end(), [&](const std::shared_ptr<PCB>& p){return p->pid==cur;}), ready_queue.end());
                    break; }
                case ProcessState::BLOCKED: {
                    blocked_processes.remove_if([&](const std::shared_ptr<PCB>& p){return p->pid==cur;});
                    break; }
                case ProcessState::RUNNING: {
                    if (current_running_process && current_running_process->pid==cur) current_running_process=nullptr;
                    break; }
                default: break;
            }

            // 加入新队列
            pcb->state = state;
            if (state==ProcessState::READY) {
                ready_queue.push_back(pcb);
            } else if (state==ProcessState::BLOCKED) {
                blocked_processes.push_back(pcb);
            } else if (state==ProcessState::RUNNING) {
                current_running_process = pcb;
            }
        }

        // ----- 传播 -----
        auto range = relations_.equal_range(cur);
        for (auto it = range.first; it != range.second; ++it) {
            const auto& [other_pid, rtype] = it->second;
            if (rtype == RelationType::SYNC) {
                // 对于同步关系，仅在 BLOCKED 与 READY 状态传播（TERMINATED 不传播）
                if (state==ProcessState::BLOCKED || state==ProcessState::READY) {
                    dfs(other_pid, state);
                }
            }
        }
    };

    dfs(pid, new_state);

    return visited.size() > 0;
}

bool ProcessManager::create_process_relationship(ProcessID pid1, ProcessID pid2, RelationType type) {
    if (!get_process(pid1) || !get_process(pid2)) return false;
    relations_.insert({pid1, {pid2, type}});
    relations_.insert({pid2, {pid1, type}});
    return true;
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
    return update_process_state(pid, ProcessState::BLOCKED);
}

bool ProcessManager::wakeup_process(ProcessID pid) {
    return update_process_state(pid, ProcessState::READY);
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

// === 进程关系获取接口实现 ===
std::vector<ProcessManager::RelationshipInfo> ProcessManager::get_all_relationships() const {
    std::vector<RelationshipInfo> rels;
    std::set<std::pair<ProcessID, ProcessID>> visited;
    for (const auto& entry : relations_) {
        ProcessID a = entry.first;
        ProcessID b = entry.second.first;
        if (visited.count({b,a})) continue; // 避免重复
        visited.insert({a,b});
        rels.push_back({a, b, entry.second.second});
    }
    return rels;
} 