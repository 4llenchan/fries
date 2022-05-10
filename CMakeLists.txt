cmake_minimum_required(VERSION 3.17)

project(fries VERSION 0.5.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(FRIES_TEST_ENABLE "Enable test target" OFF)
set(fries_dir "${CMAKE_CURRENT_SOURCE_DIR}/fries")

#set(src_file
#        )
#add_library(fries STATIC ${src_file})
#target_include_directories(fries PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(.)

if (FRIES_TEST_ENABLE)
    add_subdirectory(tests)
endif ()
