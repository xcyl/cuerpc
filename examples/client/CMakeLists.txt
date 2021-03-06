cmake_minimum_required(VERSION 2.6)

project(rpc_client)

option(ENABLE_CO_AWAIT "enable co_await" OFF)
if (ENABLE_CO_AWAIT)
    message("-- enable co_await")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -std=c++2a -stdlib=libc++ -fcoroutines-ts")
    add_definitions(-DENABLE_CO_AWAIT)
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -std=c++14")
endif ()

set(SRC main.cpp)

include_directories(${PROJECT_SOURCE_DIR}/../../include)

find_package(Boost COMPONENTS system REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

if (WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32 wsock32)
else ()
    target_link_libraries(${PROJECT_NAME} pthread)
endif ()
