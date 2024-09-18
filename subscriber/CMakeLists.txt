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

# Add differential_drive executable
add_executable(differential_drive src/differential_drive.cpp)
target_link_libraries(differential_drive ${MOTEUSAPI_LIB})
target_include_directories(differential_drive PRIVATE ${POPL_INCLUDE_DIR} ${MOTEUSAPI_INCLUDE_DIR})
