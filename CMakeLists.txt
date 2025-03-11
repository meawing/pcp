cmake_minimum_required(VERSION 3.8)

project(parallel-and-concurrent-programming)

# cmake_policy(SET CMP0135 NEW)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

set(ALLOWED_PROFILES RELEASE ASAN TSAN RELEASE_LINES ASAN_LINES)

message(STATUS "Profile: ${CMAKE_BUILD_TYPE}")

string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
# if (NOT ${CMAKE_BUILD_TYPE_UPPER} IN_LIST ALLOWED_PROFILES)
  # message(FATAL_ERROR "${CMAKE_BUILD_TYPE} profile is not allowed")
# endif()

set(CMAKE_CXX_FLAGS_ASAN "${CMAKE_CXX_FLAGS_ASAN} -O2 -fsanitize=address")
set(CMAKE_CXX_FLAGS_TSAN "${CMAKE_CXX_FLAGS_TSAN} -O2 -fsanitize=thread")

set(CMAKE_CXX_FLAGS_RELEASE_LINES "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_ASAN_LINES "${CMAKE_CXX_FLAGS_ASAN}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fno-omit-frame-pointer")

if (NOT ${CMAKE_BUILD_TYPE} MATCHES ".*LINES")
  add_definitions(-DLINES_THREADS)
endif()

# Export compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

include(cmake/Custom.cmake)
include(cmake/ThirdParty.cmake)

add_subdirectory(lines)

add_subdirectory(simple-sum)

add_subdirectory(spinlock)

add_subdirectory(mpmc-queue)
add_subdirectory(buffered-channel)
add_subdirectory(unbuffered-channel)

add_subdirectory(thread-pool)

add_subdirectory(coroutine)
add_subdirectory(fibers)

add_subdirectory(flat-combining)

add_subdirectory(std-future)
add_subdirectory(future)

add_subdirectory(futex)
add_subdirectory(memory-models)
