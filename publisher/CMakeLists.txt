cmake_minimum_required(VERSION 3.10)

project(RemoteControlPublisher)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define joystick library based on joystick.cc and joystick.hh
set(JOYSTICK_LIB joysticklib)
set(JOYSTICK_INCLUDE_DIR ../3rd/joystick)
add_library(${JOYSTICK_LIB} STATIC ../3rd/joystick/joystick.cc)
target_include_directories(${JOYSTICK_LIB} PUBLIC ${JOYSTICK_INCLUDE_DIR})

# Define popl library
set(POPL_INCLUDE_DIR ../3rd/popl/include)

# Find zenoh
find_library(ZENOH_LIB zenohc REQUIRED)
find_path(ZENOH_INCLUDE_DIR zenoh.hxx REQUIRED)
message(STATUS "Zenoh library: ${ZENOH_LIB}")
message(STATUS "Zenoh include dir: ${ZENOH_INCLUDE_DIR}")

add_compile_definitions()

# Add joystick_publisher executable
add_executable(joystick src/joystick.cpp)
target_link_libraries(joystick ${JOYSTICK_LIB} ${ZENOH_LIB})
target_include_directories(joystick PRIVATE ${JOYSTICK_INCLUDE_DIR} ${POPL_INCLUDE_DIR} ${ZENOH_INCLUDE_DIR})
target_compile_definitions(joystick PRIVATE ZENOHCXX_ZENOHC)
