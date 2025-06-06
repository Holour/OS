#include <iostream>
#include "memory/memory_manager.h"
#include "process/process_manager.h"

void print_system_status(ProcessManager& pm, MemoryManager& mm) {
    std::cout << "\n--- System Status ---" << std::endl;
    // 内存状态
    std::cout << "Memory Usage: " << mm.get_used_memory() / (1024*1024) << "MB / " 
              << mm.get_total_memory() / (1024*1024) << "MB" << std::endl;

    // 进程列表
    const auto& processes = pm.get_all_processes();
    std::cout << "Process List (" << processes.size() << " total):" << std::endl;
    for(const auto& pair : processes) {
        const auto& pcb = pair.second;
        std::cout << "  PID: " << pcb->pid 
                  << ", State: " << static_cast<int>(pcb->state)
                  << ", Mem: " << pcb->memory_info[0].size / (1024*1024) << "MB" << std::endl;
    }
    std::cout << "---------------------\n" << std::endl;
}


int main() {
    // 1. 初始化管理器
    MemoryManager mem_manager;
    mem_manager.initialize();

    ProcessManager proc_manager(mem_manager);

    // 2. 模拟创建一些进程
    std::cout << "--- Initial Process Creation ---" << std::endl;
    proc_manager.create_process(512 * 1024 * 1024); // 512MB
    proc_manager.create_process(1024 * 1024 * 1024); // 1GB
    proc_manager.create_process(2048 * 1024 * 1024); // 2GB
    
    print_system_status(proc_manager, mem_manager);

    // 3. 模拟内存不足
    std::cout << "--- Attempting to create a process that exceeds available memory ---" << std::endl;
    proc_manager.create_process(1024 * 1024 * 1024); // 1GB - 应该会失败

    print_system_status(proc_manager, mem_manager);

    // 4. 模拟调度和终止
    std::cout << "--- Scheduling and Terminating Processes ---" << std::endl;
    PCB* running_process = proc_manager.schedule();
    if(running_process) {
        std::cout << "Scheduled process " << running_process->pid << " to run." << std::endl;
        proc_manager.terminate_process(running_process->pid);
    }

    print_system_status(proc_manager, mem_manager);


    return 0;
} 