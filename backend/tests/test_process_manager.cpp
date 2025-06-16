#include "process/process_manager.h"
#include "memory/memory_manager.h"
#include "test_common.h"

void test_pm_create_process_success() {
    std::cout << "  - Testing PM Create Process Success..." << std::endl;
    MemoryManager mm;
    mm.initialize();
    ProcessManager pm(mm);

    uint64_t initial_used_memory = mm.get_used_memory();
    
    auto pid_opt = pm.create_process("test_proc", 1024, 10, 5);
    ASSERT_TRUE(pid_opt.has_value());
    auto pcb_ptr = pm.get_process(*pid_opt);
    ASSERT_NOT_NULL(pcb_ptr);
    ASSERT_EQUAL(static_cast<int>(pcb_ptr->state), static_cast<int>(ProcessState::READY));
    ASSERT_EQUAL(pm.get_all_processes().size(), 1);
    ASSERT_TRUE(mm.get_used_memory() > initial_used_memory);
    
    std::cout << "    ...PASSED" << std::endl;
}

void test_pm_create_process_oom() {
    std::cout << "  - Testing PM Create Process Out of Memory..." << std::endl;
    MemoryManager mm;
    mm.initialize();
    ProcessManager pm(mm);
    
    const uint64_t total_mem = mm.get_total_memory();
    auto pid_opt = pm.create_process("oom_proc", total_mem + 1, 10, 5);
    ASSERT_FALSE(pid_opt.has_value());
    ASSERT_EQUAL(pm.get_all_processes().size(), 0);
    
    std::cout << "    ...PASSED" << std::endl;
}

void test_pm_terminate_process() {
    std::cout << "  - Testing PM Terminate Process..." << std::endl;
    MemoryManager mm;
    mm.initialize();
    ProcessManager pm(mm);

    auto pid_opt = pm.create_process("to_terminate", 2048, 10, 5);
    ASSERT_TRUE(pid_opt.has_value());
    ProcessID pid_to_terminate = *pid_opt;
    uint64_t memory_before_termination = mm.get_used_memory();

    bool result = pm.terminate_process(pid_to_terminate);
    ASSERT_TRUE(result);
    ASSERT_NULL(pm.get_process(pid_to_terminate));
    ASSERT_TRUE(mm.get_used_memory() < memory_before_termination);
    
    // Test terminating non-existent process
    bool non_existent_result = pm.terminate_process(9999);
    ASSERT_FALSE(non_existent_result);

    std::cout << "    ...PASSED" << std::endl;
}

void test_pm_scheduler_fcfs() {
    std::cout << "  - Testing PM Scheduler (FCFS)..." << std::endl;
    MemoryManager mm;
    mm.initialize();
    ProcessManager pm(mm);
    pm.set_algorithm(SchedulingAlgorithm::FCFS);

    auto pidA_opt = pm.create_process("A", 100, 10, 1);
    auto pidB_opt = pm.create_process("B", 100, 10, 1);
    ASSERT_TRUE(pidA_opt.has_value() && pidB_opt.has_value());
    
    auto scheduled_p1 = pm.schedule();
    ASSERT_NOT_NULL(scheduled_p1);
    ASSERT_EQUAL(scheduled_p1->pid, pidA_opt.value());
    ASSERT_EQUAL(static_cast<int>(scheduled_p1->state), static_cast<int>(ProcessState::RUNNING));
    
    auto scheduled_p2 = pm.schedule();
    ASSERT_NOT_NULL(scheduled_p2);
    ASSERT_EQUAL(scheduled_p2->pid, pidB_opt.value());
    ASSERT_EQUAL(static_cast<int>(scheduled_p2->state), static_cast<int>(ProcessState::RUNNING));

    // No more processes in ready queue
    auto should_be_null = pm.schedule();
    ASSERT_NULL(should_be_null);
    
    std::cout << "    ...PASSED" << std::endl;
}

void run_process_manager_tests() {
    test_pm_create_process_success();
    test_pm_create_process_oom();
    test_pm_terminate_process();
    test_pm_scheduler_fcfs();
} 