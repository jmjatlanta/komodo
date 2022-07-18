#include <gtest/gtest.h>

#include "sync.h"

#include <thread>

CCriticalSection ccs_1;
CCriticalSection ccs_2;

TEST(test_locking, old_way)
{
    // no deadlock here, even though we unlock FIFO
    uint64_t my_val = 0;

    auto my_func = [&my_val](int thread_num) {
        for(int i = 0; i < 1000; ++i)
        {
            ENTER_CRITICAL_SECTION(ccs_1);
            ENTER_CRITICAL_SECTION(ccs_2);
            my_val += 1;
            std::cout << "In thread " << thread_num << "\n";
            LEAVE_CRITICAL_SECTION(ccs_1);
            LEAVE_CRITICAL_SECTION(ccs_2);
        }
    };

    std::thread t1(my_func, 1);
    std::thread t2(my_func, 2);

    t1.join();
    t2.join();
}

TEST(test_locking, DISABLED_old_way_with_reaquire)
{
    // deadlock here, because we unlock FIFO
    uint64_t my_val = 0;

    auto my_func = [&my_val](int thread_num) {
        for(int i = 0; i < 1000; ++i)
        {
            ENTER_CRITICAL_SECTION(ccs_1);
            ENTER_CRITICAL_SECTION(ccs_2);
            my_val += 1;
            std::cout << "In thread " << thread_num << "\n";
            LEAVE_CRITICAL_SECTION(ccs_1);
            std::cout << "Re-acquiring mutex 1 in thread " << thread_num << "\n";
            ENTER_CRITICAL_SECTION(ccs_1);
            std::cout << "Mutex 1 re-acquired for thread " << thread_num << "\n";
            LEAVE_CRITICAL_SECTION(ccs_1);
            LEAVE_CRITICAL_SECTION(ccs_2);
        }
    };

    std::thread t1(my_func, 1);
    std::thread t2(my_func, 2);

    t1.join();
    t2.join();
}

TEST(test_locking, new_way_with_reaquire)
{
    // no deadlock here, because we unlock LIFO
    uint64_t my_val = 0;

    auto my_func = [&my_val](int thread_num) {
        for(int i = 0; i < 1000; ++i)
        {
            ENTER_CRITICAL_SECTION(ccs_1);
            ENTER_CRITICAL_SECTION(ccs_2);
            my_val += 1;
            std::cout << "In thread " << thread_num << "\n";
            LEAVE_CRITICAL_SECTION(ccs_2);
            std::cout << "Re-acquiring mutex 1 in thread " << thread_num << "\n";
            ENTER_CRITICAL_SECTION(ccs_2);
            std::cout << "Mutex 1 re-acquired for thread " << thread_num << "\n";
            LEAVE_CRITICAL_SECTION(ccs_2);
            LEAVE_CRITICAL_SECTION(ccs_1);
        }
    };

    std::thread t1(my_func, 1);
    std::thread t2(my_func, 2);

    t1.join();
    t2.join();
}
