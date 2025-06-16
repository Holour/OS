#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <string>
#include <stdexcept>
#include <iostream>

// Simple assertion macros
#define ASSERT(condition) \
    if (!(condition)) { \
        throw std::runtime_error("ASSERT FAILED: " #condition " at " __FILE__ ":" + std::to_string(__LINE__)); \
    }

#define ASSERT_TRUE(condition) ASSERT(condition)
#define ASSERT_FALSE(condition) ASSERT(!(condition))
#define ASSERT_EQUAL(a, b) \
    if (!((a) == (b))) { \
        throw std::runtime_error("ASSERT FAILED: " #a " (" + std::to_string(a) + ") == " #b " (" + std::to_string(b) + ") at " __FILE__ ":" + std::to_string(__LINE__)); \
    }
#define ASSERT_NOT_EQUAL(a, b) \
    if (!((a) != (b))) { \
        throw std::runtime_error("ASSERT FAILED: " #a " (" + std::to_string(a) + ") != " #b " (" + std::to_string(b) + ") at " __FILE__ ":" + std::to_string(__LINE__)); \
    }
#define ASSERT_NULL(ptr) ASSERT((ptr) == nullptr)
#define ASSERT_NOT_NULL(ptr) ASSERT((ptr) != nullptr)

#endif // TEST_COMMON_H 