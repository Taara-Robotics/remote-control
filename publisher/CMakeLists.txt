cmake_minimum_required(VERSION 3.10)

project(RemoteControlPublisher)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define joystick library based on joystick.cc and joystick.hh
set(JOYSTICK_INCLUDE_DIR ../3rd/joystick)
add_library(joystick STATIC ../3rd/joystick/joystick.cc)
target_include_directories(joystick PUBLIC ${JOYSTICK_INCLUDE_DIR})

# Define popl library
set(POPL_INCLUDE_DIR ../3rd/popl/include)

# Add joystick_publisher executable
add_executable(joystick_publisher src/joystick_publisher.cpp)
target_link_libraries(joystick_publisher joystick)
target_include_directories(joystick_publisher PRIVATE ${JOYSTICK_INCLUDE_DIR} ${POPL_INCLUDE_DIR})
