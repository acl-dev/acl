cmake_minimum_required(VERSION 2.8.0)
project(acl)
#message(${CMAKE_SYSTEM_NAME})

#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

#The below line was built error on Android Studio
#string(TOUPPER ${CMAKE_SYSTEM_NAME} OS_NAME)
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(CMAKE_MACOSX_RPATH build)
endif()

add_subdirectory(lib_acl acl)
add_subdirectory(lib_protocol protocol)
add_subdirectory(lib_acl_cpp acl_cpp)

#if (CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "Android")
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
	add_subdirectory(lib_fiber/c fiber)
	add_subdirectory(lib_fiber/cpp fiber_cpp)
endif()
