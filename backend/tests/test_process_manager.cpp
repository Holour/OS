#include "../include/process/process_manager.h"
#include "../include/memory/memory_manager.h"
#include "../include/common.h"
#include <iostream>
#include <cassert>

void test_create_process_success(ProcessManager& pm, MemoryManager& mm) {
    std::cout << "--- Running Test: Create Process Success ---" << std::endl;
    uint64_t initial_used_memory = mm.get_used_memory();
    uint64_t initial_ready_queue_size = pm.get_ready_queue().size();
    
    PCB* pcb = pm.create_process(1024); // a 1KB process
    assert(pcb != nullptr);
    assert(pcb->state == ProcessState::READY); // Should be READY now
    assert(pm.get_all_processes().count(pcb->pid) == 1);
    assert(mm.get_used_memory() > initial_used_memory);
    assert(pm.get_ready_queue().size() == initial_ready_queue_size + 1);
    
    std::cout << "OK: Process created with PID " << pcb->pid << ", state is READY, and added to ready_queue." << std::endl;
}

void test_create_process_oom(ProcessManager& pm, MemoryManager& mm) {
    std::cout << "--- Running Test: Create Process Out of Memory ---" << std::endl;
    uint64_t initial_process_count = pm.get_all_processes().size();
    uint64_t initial_used_memory = mm.get_used_memory();

    // Try to allocate a process larger than total memory
    PCB* pcb = pm.create_process(MEMORY_SIZE + 1);
    assert(pcb == nullptr);
    assert(pm.get_all_processes().size() == initial_process_count);
    assert(mm.get_used_memory() == initial_used_memory); // Memory usage should not change
    
    std::cout << "OK: Process creation failed as expected due to OOM." << std::endl;
}

void test_terminate_process_success(ProcessManager& pm, MemoryManager& mm) {
    std::cout << "--- Running Test: Terminate Process Success ---" << std::endl;
    PCB* pcb = pm.create_process(2048);
    assert(pcb != nullptr);
    
    pid_t pid_to_terminate = pcb->pid;
    uint64_t memory_before_termination = mm.get_used_memory();
    uint64_t initial_ready_queue_size = pm.get_ready_queue().size();

    bool result = pm.terminate_process(pid_to_terminate);
    assert(result == true);
    assert(pm.get_all_processes().count(pid_to_terminate) == 0);
    assert(mm.get_used_memory() < memory_before_termination);
    assert(pm.get_ready_queue().size() == initial_ready_queue_size - 1);
    
    std::cout << "OK: Process " << pid_to_terminate << " terminated, removed from queues and memory was freed." << std::endl;
}

void test_terminate_nonexistent_process(ProcessManager& pm) {
    std::cout << "--- Running Test: Terminate Non-existent Process ---" << std::endl;
    pid_t non_existent_pid = 9999; // A PID that is unlikely to exist
    bool result = pm.terminate_process(non_existent_pid);
    assert(result == false);

    std::cout << "OK: Terminating non-existent process failed as expected." << std::endl;
}

void test_scheduler(ProcessManager& pm) {
    std::cout << "--- Running Test: Scheduler ---" << std::endl;
    
    // Create two processes, they will be added to the ready queue automatically
    PCB* p1 = pm.create_process(100);
    PCB* p2 = pm.create_process(100);
    assert(p1 != nullptr && p2 != nullptr);
    
    assert(pm.get_ready_queue().size() >= 2);
    
    PCB* scheduled_p1 = pm.tick_schedule();
    assert(scheduled_p1 == p1); // Simple FIFO scheduling
    assert(scheduled_p1->state == ProcessState::RUNNING);
    assert(pm.get_ready_queue().size() >= 1);
    
    PCB* scheduled_p2 = pm.tick_schedule();
    assert(scheduled_p2 == p2);
    assert(scheduled_p2->state == ProcessState::RUNNING);

    PCB* should_be_null = pm.tick_schedule();
    if (pm.get_ready_queue().empty()) {
       assert(should_be_null == nullptr);
    }
    
    std::cout << "OK: Scheduler correctly selected processes in FIFO order." << std::endl;
}

void cleanup_processes(ProcessManager& pm) {
    // Need to copy pids because terminate_process modifies the map we are iterating over
    std::vector<pid_t> pids;
    for(const auto& pair : pm.get_all_processes()) {
        pids.push_back(pair.first);
    }
    for(pid_t pid : pids) {
        pm.terminate_process(pid);
    }
}

int main() {
    std::cout << "===== Starting ProcessManager Tests =====" << std::endl;

    // 1. Initialize dependencies
    MemoryManager memory_manager;
    memory_manager.initialize();
    ProcessManager process_manager(memory_manager);

    // 2. Run test cases
    test_create_process_success(process_manager, memory_manager);
    cleanup_processes(process_manager);

    test_create_process_oom(process_manager, memory_manager);
    cleanup_processes(process_manager);

    test_terminate_process_success(process_manager, memory_manager);
    cleanup_processes(process_manager);
    
    test_terminate_nonexistent_process(process_manager);
    cleanup_processes(process_manager);

    test_scheduler(process_manager);
    cleanup_processes(process_manager);

    std::cout << "===== All ProcessManager Tests Passed! =====" << std::endl;
    return 0;
} 