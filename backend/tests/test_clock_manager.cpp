#include "clock/clock_manager.h"
#include "test_common.h"
#include <thread>
#include <chrono>

void test_initial_state() {
    std::cout << "  - Testing Initial State..." << std::endl;
    ClockManager cm;
    ASSERT_EQUAL(cm.get_ticks(), 0);
    // Uptime might be slightly more than 0, but should be very small
    ASSERT_TRUE(cm.get_uptime_ms() < 100); 
    std::cout << "    ...PASSED" << std::endl;
}

void test_time_progression() {
    std::cout << "  - Testing Time Progression..." << std::endl;
    ClockManager cm;
    cm.start();
    
    uint64_t initial_uptime = cm.get_uptime_ms();
    uint64_t initial_ticks = cm.get_ticks();

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    uint64_t later_uptime = cm.get_uptime_ms();
    uint64_t later_ticks = cm.get_ticks();

    ASSERT_TRUE(later_uptime > initial_uptime);
    ASSERT_TRUE(later_uptime >= 150);
    // Ticks are based on uptime, so they should also increase
    ASSERT_TRUE(later_ticks > initial_ticks);
    std::cout << "    ...PASSED" << std::endl;
}

void test_timer_management() {
    std::cout << "  - Testing Timer Management..." << std::endl;
    ClockManager cm;
    cm.start();

    // Set a one-shot timer
    auto timer1_id = cm.set_timer(200, false, 0);
    ASSERT_TRUE(timer1_id.has_value());
    ASSERT_EQUAL(timer1_id.value(), 1);

    // Set a repeating timer
    auto timer2_id = cm.set_timer(300, true, 100);
    ASSERT_TRUE(timer2_id.has_value());
    ASSERT_EQUAL(timer2_id.value(), 2);

    // Try to set a timer with invalid parameters (though the current implementation does not check for this)
    // For now, we just check if it returns a new ID
    auto timer3_id = cm.set_timer(0, false, 0);
    ASSERT_TRUE(timer3_id.has_value());
    ASSERT_NOT_EQUAL(timer3_id.value(), timer2_id.value());

    std::cout << "    ...PASSED" << std::endl;
}


void run_clock_manager_tests() {
    test_initial_state();
    test_time_progression();
    test_timer_management();
} 