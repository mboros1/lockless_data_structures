
enable_testing()

add_executable(queue_tests queue_tests.cpp)

target_link_libraries(
        queue_tests
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(queue_tests)
