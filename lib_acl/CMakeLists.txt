cmake_minimum_required(VERSION 2.8)

#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

#message(project_dir=${PROJECT_SOURCE_DIR})

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

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
	"-Wno-invalid-source-encoding"
	"-Wstrict-prototypes"
)

if (CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE "DEBUG")
else()
	string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
	add_definitions("-O3")
else()
	add_definitions("-g")
endif()

#string(TOUPPER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME)
if (CMAKE_SYSTEM_NAME MATCHES "Android")
	add_definitions("-DANDROID")
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
else()
	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

##############################################################################

set(src ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(sources
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
	${src}/master/framework
	${src}/master/framework/trigger
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

if (CMAKE_SYSTEM_NAME MATCHES "Android")
	set(CMAKE_SHARED_LINKER_FLAGS "-shared")
else()
	set(lib_output_path ${PROJECT_BINARY_DIR}/lib)

	set(LIBRARY_OUTPUT_PATH ${lib_output_path})
	add_library(acl_static STATIC ${lib_src})
	SET_TARGET_PROPERTIES(acl_static PROPERTIES OUTPUT_NAME "acl")

	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})
endif()

add_library(acl SHARED ${lib_src})

##############################################################################
