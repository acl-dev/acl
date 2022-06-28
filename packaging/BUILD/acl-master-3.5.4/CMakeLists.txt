cmake_minimum_required(VERSION 2.8.0)
project(Acl)
#message(${CMAKE_SYSTEM_NAME})

#if(COMMAND CMAKE_POLICY)
#    message(STATUS "Set CMake Policy")
#    if(CMAKE_VERSION GREATER "2.8")
#        CMAKE_POLICY(set CMP0015 OLD)
#        CMAKE_POLICY(set CMP0010 OLD)
#        CMAKE_POLICY(set CMP0011 OLD)
#    endif(CMAKE_VERSION GREATER "2.8")
#endif(COMMAND CMAKE_POLICY)

enable_testing()

#include(CheckCCompilerFlag)
#check_c_compiler_flag("-std=c99" COMPILER_SUPPORTS_C99)
#check_c_compiler_flag("-std=c89" COMPILER_SUPPORTS_C89)
#if(COMPILER_SUPPORTS_C99)
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
#elseif(COMPILER_SUPPORTS_C89)
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c89")
#else()
#    message(STATUS "The cc compiler ${CMAKE_C_COMPILER} has no C99 or C89 support. Please use a different cc compiler.")
#endif()

#include(CheckCXXCompilerFlag)
#check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
#check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
#if(COMPILER_SUPPORTS_CXX11)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#elseif(COMPILER_SUPPORTS_CXX0X)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
#else()
#    message(STATUS "The c++ compiler ${CMAKE_CXX_COMPILER} has no C++11 or C++0x support. Please use a different c++ compiler.")
#endif()

#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

#string(TOUPPER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME)
if (CMAKE_SYSTEM_NAME MATCHES "Android")
	message(STATUS "current platform: Android")
	message(STATUS "current ndk path: ${CMAKE_ANDROID_NDK}")
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
	message(STATUS "current platform: Linux")
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
	message(STATUS "current platform: Darwin")
	set(CMAKE_MACOSX_RPATH build)
elseif (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	message(STATUS "current platform: FreeBSD")
	include_directories(
		#/usr/include/c++/4.2
		#/usr/include/c++/4.2/backward
		/usr/local/include
		/usr/include
	)

	link_directories(
		/usr/lib
		/usr/local/lib
	)

	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--export-dynamic")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,./:./lib:../:../lib")
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
	message(STATUS "current platform: Windows")
else()
	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(CMAKE_MACOSX_RPATH build)
endif()

add_subdirectory(lib_acl acl)
add_subdirectory(lib_protocol protocol)
add_subdirectory(lib_acl_cpp acl_cpp)
if (ACL_BUILD_SHARED MATCHES "YES")
	add_dependencies(protocol acl)
	add_dependencies(acl_cpp protocol acl)
endif()

#if (CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "Android")
#if (CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "Darwin")
	add_subdirectory(lib_fiber/c fiber)
	add_subdirectory(lib_fiber/cpp fiber_cpp)
	if (ACL_BUILD_SHARED MATCHES "YES")
		add_dependencies(fiber_cpp acl_cpp protocol acl fiber)
	endif()
#endif()
