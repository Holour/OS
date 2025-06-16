#include "interrupt/interrupt_manager.h"
#include "process/process_manager.h"
#include "memory/memory_manager.h"
#include "test_common.h"
#include <iostream>

// We need real instances because the base class methods are not virtual.
static MemoryManager test_mem_manager;
static ProcessManager test_proc_manager(test_mem_manager);

void test_handler_registration_real() {
    std::cout << "  - Testing Handler Registration (Real)..." << std::endl;
    // InterruptManager depends on a ProcessManager
    InterruptManager im(test_proc_manager);

    bool registered = im.register_handler(32, "TIMER", 1);
    ASSERT_TRUE(registered);

    // Try to register again on the same vector
    bool registered_again = im.register_handler(32, "IO", 2);
    ASSERT_FALSE(registered_again); // Should fail if vector is already taken

    auto table = im.get_vector_table();
    ASSERT_EQUAL(table.size(), 1);
    ASSERT_EQUAL(table[0].first, 32);
    ASSERT_TRUE(table[0].second.type == "TIMER");
    ASSERT_EQUAL(table[0].second.priority, 1);

    std::cout << "    ...PASSED" << std::endl;
}

void test_interrupt_triggering_real() {
    std::cout << "  - Testing Interrupt Triggering (Real)..." << std::endl;
    InterruptManager im(test_proc_manager);
    
    // Test triggering without a registered handler
    bool triggered_nonexistent = im.trigger_interrupt(40, {});
    ASSERT_FALSE(triggered_nonexistent);

    // Register a handler and then trigger it
    im.register_handler(33, "SYSCALL", 5);
    bool triggered_existent = im.trigger_interrupt(33, {{"syscall_id", 1}});
    ASSERT_TRUE(triggered_existent);
    
    std::cout << "    ...PASSED" << std::endl;
}

// Renaming the main test function to avoid conflicts
void run_interrupt_manager_tests() {
    // Initialize managers for a clean state in each test run if necessary
    test_mem_manager.initialize();
    
    test_handler_registration_real();
    test_interrupt_triggering_real();
} 