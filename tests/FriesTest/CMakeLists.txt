cmake_minimum_required(VERSION 3.17)
project(FriesTest)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        main.cpp
        )
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../ fries_binary_dir)
include_directories(../../)
add_executable(FriesTest ${SOURCE_FILES})