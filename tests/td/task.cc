#include <doctest.hh>

#include <array>
#include <iostream>
#include <limits>
#include <memory>

#include <task-dispatcher/common/system_info.hh>
#include <task-dispatcher/container/task.hh>

namespace
{
int gSink = 0;

void test_func(void* userdata) { gSink += *static_cast<int*>(userdata); }
}

TEST_CASE("td::container::Task (lifetime)")
{
    // Constants are not constexpr variables because
    // lambda captures are a concern of this test.
    // Could be randomized.
#define SHARED_INT_VALUE 55

    td::container::Task task;
    std::weak_ptr<int> weakInt;

    for (auto i = 0; i < 3; ++i)
    {
        bool taskExecuted = false;

        {
            std::shared_ptr<int> sharedInt = std::make_shared<int>(SHARED_INT_VALUE);
            weakInt = sharedInt;

            task.lambda([sharedInt, &taskExecuted]() {
                // Check if sharedInt is alive and correct
                CHECK_EQ(*sharedInt, SHARED_INT_VALUE);

                taskExecuted = true;
            });

            // sharedInt runs out of scope
        }

        // Check if task kept the lambda capture alive
        CHECK(!weakInt.expired());
        CHECK(!taskExecuted);

        task.execute();

        CHECK(taskExecuted);
        CHECK(!weakInt.expired());

        task.cleanup();

        // Check if execute_and_cleanup destroyed the lambda capture
        CHECK(taskExecuted);
        CHECK(weakInt.expired());
    }
#undef SHARED_INT_VALUE
}

TEST_CASE("td::container::Task (static)")
{
    static_assert(sizeof(td::container::Task) == td::system::l1_cacheline_size);

    // Lambdas
    {
        int constexpr a = 0, b = 1, c = 2;
        int x = 3, y = 4;
        auto uptr = std::make_unique<int>(1);

        auto l_trivial = [] { ++gSink; };
        auto l_ref_cap = [&] { gSink += (a - b + c); };
        auto l_val_cap = [=] { gSink += (a - b + c); };
        auto l_val_cap_mutable = [=]() mutable { gSink += (x += y); };
        auto l_noexcept = [&]() noexcept { gSink += (a - b + c); };
        auto l_constexpr = [=]() constexpr { return a - b + c; };
        auto l_noncopyable = [p = std::move(uptr)] { gSink += *p; };

        // Test if these lambda types compile
        td::container::Task(std::move(l_trivial)).executeAndCleanup();
        td::container::Task(std::move(l_ref_cap)).executeAndCleanup();
        td::container::Task(std::move(l_val_cap)).executeAndCleanup();
        td::container::Task(std::move(l_val_cap_mutable)).executeAndCleanup();
        td::container::Task(std::move(l_noexcept)).executeAndCleanup();
        td::container::Task(std::move(l_constexpr)).executeAndCleanup();
        td::container::Task(std::move(l_noncopyable)).executeAndCleanup();
    }

    // Function pointers
    {
        {
            gSink = 0;
            auto expected_stack = 0;
            auto stack_increment = 5;

            auto const check_increment = [&]() {
                expected_stack += stack_increment;
                CHECK_EQ(gSink, expected_stack);
            };

            auto l_decayable = [](void* userdata) { gSink += *static_cast<int*>(userdata); };

            CHECK_EQ(gSink, expected_stack);

            td::container::Task(l_decayable, &stack_increment).executeAndCleanup();
            check_increment();

            td::container::Task([](void* userdata) { gSink += *static_cast<int*>(userdata); }, &stack_increment).executeAndCleanup();
            check_increment();

            td::container::Task(+[](void* userdata) { gSink += *static_cast<int*>(userdata); }, &stack_increment).executeAndCleanup();
            check_increment();

            td::container::Task(test_func, &stack_increment).executeAndCleanup();
            check_increment();
        }

        // Function pointer variant, takes lambdas and function pointers void(void*)
        td::container::Task([](void*) {}).executeAndCleanup();
        td::container::Task(+[](void*) {}).executeAndCleanup();
        td::container::Task([](void*) {}, nullptr).executeAndCleanup();
        td::container::Task(+[](void*) {}, nullptr).executeAndCleanup();

        // Lambda variant, takes lambdas and function pointers void()
        td::container::Task([] {}).executeAndCleanup();
        td::container::Task(+[] {}).executeAndCleanup();
        //td::container::Task([] {}, nullptr).executeAndCleanup(); // ERROR
        //td::container::Task(+[] {}, nullptr).executeAndCleanup(); // ERROR
    }
}

TEST_CASE("td::container::Task (metadata)")
{
    // Constants are not constexpr variables because
    // lambda captures are a concern of this test.
    // Could be randomized.
#define TASK_CANARAY_INITIAL 20
#define CAPTURE_PAD_SIZE 32

    td::container::Task task;

    using metadata_t = td::container::Task::default_metadata_t;
    auto constexpr metaMin = std::numeric_limits<metadata_t>().min();
    auto constexpr metaMax = std::numeric_limits<metadata_t>().max();
    for (auto testMetadata : {metaMin, metadata_t(0), metaMax})
    {
        uint16_t taskRunCanary = TASK_CANARAY_INITIAL;

        std::array<char, CAPTURE_PAD_SIZE> pad;
        std::fill(pad.begin(), pad.end(), 0);

        task.setMetadata(testMetadata);

        // Test if the write was successful
        CHECK(task.getMetadata() == testMetadata);

        task.lambda([testMetadata, &taskRunCanary, pad]() {
            // Sanity
            CHECK(taskRunCanary == TASK_CANARAY_INITIAL);

            taskRunCanary = testMetadata;

            for (auto i = 0u; i < CAPTURE_PAD_SIZE; ++i)
            {
                // Check if the pad does not collide with the metadata
                CHECK(pad.at(i) == 0);
            }
        });

        // Test if the task write didn't compromise the metadata
        CHECK(task.getMetadata() == testMetadata);

        // Sanity
        CHECK(taskRunCanary == TASK_CANARAY_INITIAL);

        task.executeAndCleanup();

        // Test if the task ran correctly
        CHECK(taskRunCanary == testMetadata);

        // Test if the metadata survived
        CHECK(task.getMetadata() == testMetadata);
    }

#undef TASK_CANARAY_INITIAL
#undef CAPTURE_PAD_SIZE
}