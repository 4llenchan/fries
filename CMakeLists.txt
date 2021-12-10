cmake_minimum_required(VERSION 3.17)

project(fries VERSION 0.3.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(FRIES_TEST_ENABLE "Enable test target" OFF)
set(FRIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fries")

include_directories(.)

if (FRIES_TEST_ENABLE)
    set(FRIES_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    add_subdirectory(tests)
endif ()
