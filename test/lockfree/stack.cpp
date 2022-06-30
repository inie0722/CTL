#include <iostream>
#include <thread>
#include <cstddef>
#include <vector>

#include <gtest/gtest.h>

extern "C"
{
#include "CTL/lockfree/stack.h"
}

constexpr size_t COUNT = 10000;

constexpr size_t MAX_SIZE = 256;

constexpr size_t THREAD_WRITE_NUM = 2;

constexpr size_t THREAD_READ_NUM = 2;

class verify
{
public:
    template <size_t DATA_SIZE>
    struct value
    {
        size_t val;
        char _[DATA_SIZE - sizeof(val)];
    };

    template <size_t DATA_SIZE>
    void run_one()
    {
        using value_t = value<DATA_SIZE>;

        CTL_lockfree_stack stack;
        CTL_lockfree_stack_new(&stack, sizeof(value_t));

        std::vector<std::atomic<size_t>> array(COUNT);

        std::atomic<size_t> write_diff = 0;
        std::atomic<size_t> read_diff = 0;

        std::thread write_thread[THREAD_WRITE_NUM];
        std::thread read_thread[THREAD_READ_NUM];

        for (size_t i = 0; i < THREAD_WRITE_NUM; i++)
        {
            write_thread[i] = std::thread([&]()
                                          {
                value_t data;

                auto start = std::chrono::steady_clock::now();
                for (size_t i = 0; i < COUNT; i++)
                {
                    data.val = i;

                    CTL_lockfree_stack_push(&stack, &data);
                        
                }
                auto end = std::chrono::steady_clock::now();
                write_diff += (end - start).count(); });
        }

        for (size_t i = 0; i < THREAD_READ_NUM; i++)
        {
            read_thread[i] = std::thread([&]()
                                         {
               value_t data;

                auto start = std::chrono::steady_clock::now();
                for (size_t i = 0; i < COUNT; i++)
                {
                CTL_lockfree_stack_pop(&stack, &data);
                    size_t index = data.val;
                    array[index]++;
                }
                auto end = std::chrono::steady_clock::now();
                read_diff += (end - start).count(); });
        }

        for (size_t i = 0; i < THREAD_WRITE_NUM; i++)
        {
            write_thread[i].join();
        }

        for (size_t i = 0; i < THREAD_READ_NUM; i++)
        {
            read_thread[i].join();
        }

        size_t max = 0;
        size_t min = 0;
        for (size_t i = 0; i < COUNT; i++)
        {
            if (array[i] != THREAD_WRITE_NUM)
            {
                if (array[i] > THREAD_WRITE_NUM)
                    max++;
                else
                    min++;
            }
        }
        std::cout << "size/" << DATA_SIZE << " byte\t"
                  << "w/" << (write_diff / THREAD_WRITE_NUM / COUNT)
                  << " ns\t"
                  << "r/" << read_diff / THREAD_READ_NUM / COUNT
                  << " ns\t" << std::endl;

        ASSERT_TRUE(max == 0);
        ASSERT_TRUE(min == 0);

        CTL_lockfree_stack_delete(&stack);
    }

    template <size_t... DATA_SIZE_>
    void run()
    {
        (run_one<DATA_SIZE_>(), ...);
    }
};

TEST(stack, stack)
{
    CTL_lockfree_stack stack;
    CTL_lockfree_stack_new(&stack, sizeof(int));

    int a = rand();
    CTL_lockfree_stack_push(&stack, &a);
    ASSERT_TRUE(CTL_lockfree_stack_size(&stack) == 1);
    ASSERT_FALSE(CTL_lockfree_stack_empty(&stack));

    int b = 0;
    CTL_lockfree_stack_pop(&stack, &b);
    ASSERT_TRUE(b == a);
    ASSERT_TRUE(CTL_lockfree_stack_size(&stack) == 0);
    ASSERT_TRUE(CTL_lockfree_stack_empty(&stack));

    int array_a[5] = {rand(), rand(), rand(), rand(), rand()};

    for (size_t i = 0; i < 5; i++)
    {
        CTL_lockfree_stack_push(&stack, &array_a[i]);
    }

    ASSERT_TRUE(CTL_lockfree_stack_size(&stack) == 5);
    ASSERT_FALSE(CTL_lockfree_stack_empty(&stack));

    int array_b[5] = {0};

    for (size_t i = 0; i < 5; i++)
    {
        CTL_lockfree_stack_pop(&stack, &array_b[i]);
    }

    ASSERT_TRUE(b == a);
    ASSERT_TRUE(CTL_lockfree_stack_size(&stack) == 0);
    ASSERT_TRUE(CTL_lockfree_stack_empty(&stack));

    CTL_lockfree_stack_delete(&stack);
}

TEST(stack, performance)
{
    verify v;
    v.run<64, 128, 256, 512, 1024>();
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}