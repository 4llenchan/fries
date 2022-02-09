add_executable(fries_test main.cpp
        ${fries_test_dir}/futures/future_test.cpp
        ${fries_test_dir}/system/switcher_test.cpp)

enable_testing()
find_package(GTest REQUIRED)
target_link_libraries(fries_test PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main)
gtest_discover_tests(fries_test)