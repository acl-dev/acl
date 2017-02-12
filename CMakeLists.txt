cmake_minimum_required(VERSION 2.8)
project(acl)
message(${CMAKE_SYSTEM_NAME})

#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

add_subdirectory(lib_acl lib_acl)
add_subdirectory(lib_protocol lib_protocol)
add_subdirectory(lib_acl_cpp lib_acl_cpp)

#if (CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "Android")
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
	add_subdirectory(lib_fiber/c lib_fiber)
	add_subdirectory(lib_fiber/cpp lib_fiber_cpp)
endif()
