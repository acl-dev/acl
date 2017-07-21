cmake_minimum_required(VERSION 2.8)

#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

#message(project_dir=${PROJECT_SOURCE_DIR})

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

#string(TOUPPER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME)
if (CMAKE_SYSTEM_NAME MATCHES "Android")
	add_definitions("-DANDROID")
	set(UNIX_OS true)
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(UNIX_OS true)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(UNIX_OS true)
elseif (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	set(UNIX_OS true)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(WIN_OS true)
else()
	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

##############################################################################

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(src ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(sources
	${src}
	${src}/stdlib
	${src}/stdlib/debug
	${src}/stdlib/memory
	${src}/stdlib/filedir
	${src}/stdlib/string
	${src}/stdlib/common
	${src}/stdlib/sys
	${src}/stdlib/sys/unix
	${src}/stdlib/configure
	${src}/stdlib/iostuff
	${src}/aio
	${src}/code
	${src}/db
	${src}/db/memdb
	${src}/db/mysql
	${src}/db/null
	${src}/db/zdb
	${src}/event
	${src}/init
	${src}/ioctl
	${src}/json
	${src}/master
	${src}/master/template
	${src}/msg
	${src}/net
	${src}/net/dns
	${src}/net/connect
	${src}/net/listen
	${src}/private
	${src}/thread
	${src}/unit_test
	${src}/xml
)

foreach(iter ${sources})
	aux_source_directory(${iter} lib_src)
endforeach()

##############################################################################

if (CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE "DEBUG")
else()
	string(TOUPPER CMAKE_BUILD_TYPE CMAKE_BUILD_TYPE)
endif()

if (${UNIX_OS})
	add_definitions(
		"-W"
		"-O2"
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

	if (!CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
		add_definitions("-Wno-invalid-source-encoding")
	endif()

	if (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
		add_definitions("-O3")
	else()
		add_definitions("-g")
	endif()
elseif (${WIN_OS})
	add_definitions(
		"-Yc"
		"-DACL_WRITEABLE_CHECK"
		"-DACL_PREPARE_COMPILE")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Android")
	set(CMAKE_SHARED_LINKER_FLAGS "-shared")
	add_definitions("-Wno-invalid-source-encoding")
else()
	set(lib_output_path ${PROJECT_BINARY_DIR}/lib)

	set(LIBRARY_OUTPUT_PATH ${lib_output_path})
	add_library(acl_static STATIC ${lib_src})
	SET_TARGET_PROPERTIES(acl_static PROPERTIES OUTPUT_NAME "acl")

	if (${WIN_OS})
		set(CMAKE_SHARED_LINKER_FLAGS "/DYNAMICBASE ws2_32.lib IPHlpApi.lib")
	endif()

	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})
endif()

add_library(acl SHARED ${lib_src})

##############################################################################
