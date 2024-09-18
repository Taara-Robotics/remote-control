cmake_minimum_required(VERSION 3.10)

project(RemoteControlSubscriber)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define moteusapi library
set(MOTEUSAPI_LIB moteusapi)
set(MOTEUSAPI_INCLUDE_DIR ../3rd/moteusapi)

add_library(${MOTEUSAPI_LIB} STATIC ../3rd/moteusapi/MoteusAPI.cpp)
target_include_directories(${MOTEUSAPI_LIB} PUBLIC ${MOTEUSAPI_INCLUDE_DIR})

# Define popl library
set(POPL_INCLUDE_DIR ../3rd/popl/include)

# Find zenoh
find_library(ZENOH_LIB zenohc REQUIRED)
find_path(ZENOH_INCLUDE_DIR zenoh.hxx REQUIRED)
message(STATUS "Zenoh library: ${ZENOH_LIB}")
message(STATUS "Zenoh include dir: ${ZENOH_INCLUDE_DIR}")

# Add differential_drive executable
add_executable(differential_drive src/differential_drive.cpp)
target_link_libraries(differential_drive ${MOTEUSAPI_LIB} ${ZENOH_LIB})
target_include_directories(differential_drive PRIVATE ${POPL_INCLUDE_DIR} ${MOTEUSAPI_INCLUDE_DIR} ${ZENOH_INCLUDE_DIR})
target_compile_definitions(differential_drive PRIVATE ZENOHCXX_ZENOHC)
