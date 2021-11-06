cmake_minimum_required(VERSION 3.17)

add_executable(fries_test main.cpp
        ${FRIES_DIR}/futures/tests/future_test.cpp)

enable_testing()
find_package(GTest REQUIRED)
target_link_libraries(fries_test PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main)
gtest_discover_tests(fries_test)