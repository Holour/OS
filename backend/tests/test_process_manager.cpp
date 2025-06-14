#include "../include/process/process_manager.h"
#include "../include/memory/memory_manager.h"
#include "../include/common.h"
#include <iostream>
#include <cassert>

void test_create_process_success(ProcessManager& pm, MemoryManager& mm) {
    std::cout << "--- Running Test: Create Process Success ---" << std::endl;
    uint64_t initial_used_memory = mm.get_used_memory();
    uint64_t initial_ready_queue_size = pm.get_ready_queue().size();
    
    auto pid_opt = pm.create_process(1024);
    assert(pid_opt.has_value());
    auto pcb = pm.get_process(*pid_opt).get();
    assert(pcb != nullptr);
    assert(pcb->state == ProcessState::READY);
    assert(pm.get_all_processes().size() == 1);
    assert(mm.get_used_memory() > initial_used_memory);
    assert(pm.get_ready_queue().size() == initial_ready_queue_size + 1);
    
    std::cout << "OK: Process created with PID " << pcb->pid << ", state is READY, and added to ready_queue." << std::endl;
}

void test_create_process_oom(ProcessManager& pm, MemoryManager& mm) {
    std::cout << "--- Running Test: Create Process Out of Memory ---" << std::endl;
    uint64_t initial_process_count = pm.get_all_processes().size();
    uint64_t initial_used_memory = mm.get_used_memory();

    auto pid_opt = pm.create_process(MEMORY_SIZE + 1);
    assert(!pid_opt.has_value());
    assert(pm.get_all_processes().size() == initial_process_count);
    assert(mm.get_used_memory() == initial_used_memory); // Memory usage should not change
    
    std::cout << "OK: Process creation failed as expected due to OOM." << std::endl;
}

void test_terminate_process_success(ProcessManager& pm, MemoryManager& mm) {
    std::cout << "--- Running Test: Terminate Process Success ---" << std::endl;
    auto pid_opt2 = pm.create_process(2048);
    assert(pid_opt2.has_value());
    auto pcb2 = pm.get_process(*pid_opt2).get();
    auto pid_to_terminate = pcb2->pid;
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
    
    auto pidA = pm.create_process(100).value();
    auto pidB = pm.create_process(100).value();
    auto p1 = pm.get_process(pidA).get();
    auto p2 = pm.get_process(pidB).get();
    assert(p1 != nullptr && p2 != nullptr);
    
    assert(pm.get_ready_queue().size() >= 2);
    
    auto scheduled_p1 = pm.tick_schedule();
    assert(scheduled_p1 && scheduled_p1->pid == p1->pid);
    assert(scheduled_p1->state == ProcessState::RUNNING);
    assert(pm.get_ready_queue().size() >= 1);
    
    auto scheduled_p2 = pm.tick_schedule();
    assert(scheduled_p2 && scheduled_p2->pid == p2->pid);
    assert(scheduled_p2->state == ProcessState::RUNNING);

    auto maybe_null = pm.tick_schedule();
    if (pm.get_ready_queue().empty()) {
        assert(!maybe_null);
    }
    
    std::cout << "OK: Scheduler correctly selected processes in FIFO order." << std::endl;
}

void cleanup_processes(ProcessManager& pm) {
    std::vector<pid_t> pids;
    for(const auto& pcb_ptr : pm.get_all_processes()) {
        pids.push_back(pcb_ptr->pid);
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