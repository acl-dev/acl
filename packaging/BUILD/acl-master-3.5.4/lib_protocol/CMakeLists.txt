cmake_minimum_required(VERSION 2.8)
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

#string(TOUPPER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME)
if (CMAKE_SYSTEM_NAME MATCHES "Android")
	# xxx: -flto can't be used on NDK with gnustl_shared
	if (ANDROID_STL MATCHES "gnustl_shared")
		add_definitions("-Oz -g -DHAVE_NO_ATEXIT")
	else()
		add_definitions("-O3 -flto")
	endif()
	add_definitions("-DANDROID")
#	add_definitions("-Wno-invalid-source-encoding")
	add_definitions("-Wno-unused-command-line-argument")
	add_definitions("-fdata-sections -ffunction-sections")
	string(APPEND CMAKE_C_FLAGS "-Qunused-arguments")
	set(UNIX_OS true)
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
	add_definitions("-O2")
	set(UNIX_OS true)
elseif (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	add_definitions("-O2")
	set(UNIX_OS true)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
	add_definitions("-Wno-invalid-source-encoding")
	add_definitions("-fdata-sections -ffunction-sections")
	add_definitions("-flto")
	add_definitions("-Os")
	set(UNIX_OS true)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(CMAKE_SHARED_LINKER_FLAGS "/DYNAMICBASE ws2_32.lib")
	set(WIN_OS true)
else()
	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

if (ACL_CLIENT_ONLY MATCHES "YES")
	add_definitions("-DACL_CLIENT_ONLY")
	message(STATUS "protocol: ACL_CLIENT_ONLY been set")
endif()

##############################################################################

set(acl_path ${CMAKE_CURRENT_SOURCE_DIR}/../lib_acl)
set(acl_include ${acl_path}/include)

include_directories(
	${acl_include}
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(src ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(sources
	${src}
	${src}/http
	${src}/smtp
	${src}/icmp
)

foreach (iter ${sources})
	aux_source_directory(${iter} lib_src)
endforeach()

##############################################################################

if (${UNIX_OS})
	add_definitions(
		"-W"
		"-fPIC"
		"-Wall"
		"-Werror"
		"-Wshadow"
		"-Wpointer-arith"
		"-Waggregate-return"
		"-Wmissing-prototypes"
		"-D_REENTRANT"
		"-D_USE_FAST_MACRO"
		"-DACL_WRITEABLE_CHECK"
		"-Wno-long-long"
		"-Wuninitialized"
		"-D_POSIX_PTHREAD_SEMANTICS"
		"-DACL_PREPARE_COMPILE"
		"-Wstrict-prototypes"
		)

#	if (CMAKE_BUILD_TYPE STREQUAL "")
#		set(CMAKE_BUILD_TYPE "DEBUG")
#	else()
#		string(TOUPPER CMAKE_BUILD_TYPE CMAKE_BUILD_TYPE)
#	endif()

	if (!CMAKE_BUILD_TYPE STREQUAL "RELEASE")
		add_definitions("-g")
	endif()

elseif (${WIN_OS})
	add_definitions(
		"-Yc"
		"-D_WINSOCK_DEPRECATED_NO_WARNINGS"
		"-DACL_WRITEABLE_CHECK"
		"-DACL_PREPARE_COMPILE")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Android")
	set(lib_output_path ${CMAKE_CURRENT_SOURCE_DIR}/../android/lib/${ANDROID_ABI})
else()
	set(lib_output_path ${PROJECT_BINARY_DIR}/lib)
endif()

set(LIBRARY_OUTPUT_PATH ${lib_output_path})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})

add_library(protocol_static STATIC ${lib_src})
SET_TARGET_PROPERTIES(protocol_static PROPERTIES OUTPUT_NAME "protocol")
link_directories(${lib_output_path})

if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND ACL_BUILD_SHARED MATCHES "YES")
	if (CMAKE_SYSTEM_NAME MATCHES "Android")
		if (ANDROID_STL MATCHES "gnustl_shared")
			set(sys_ldflags "-shared -lz")
		else()
			set(sys_ldflags "-shared -flto -lz")
		endif()
		target_compile_options(protocol_static PRIVATE -fvisibility=hidden)
	elseif (${UNIX_OS})
		set(sys_ldflags "-shared -lz -lpthread -ldl")
		target_compile_options(protocol_static PRIVATE -fvisibility=hidden)
	endif()

	if (ACL_BUILD_SHARED_ONE MATCHES "YES")
		set(lc_flags "${lib_output_path}/libacl.a")
		set(CMAKE_SHARED_LINKER_FLAGS "${lc_flags} ${sys_ldflags}")
		add_library(protocol SHARED ${lib_src})
	else()
		set(lc_flags "-Wl,-rpath,. -L${lib_output_path} -lacl")
		set(CMAKE_SHARED_LINKER_FLAGS "${lc_flags} ${sys_ldflags}")
		add_library(protocol SHARED ${lib_src})
		target_link_libraries(protocol acl)
	endif()
endif()

##############################################################################
