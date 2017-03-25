cmake_minimum_required(VERSION 2.8)
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

add_definitions(
	"-W"
	"-fPIC"
	"-Wall"
	"-Werror"
	"-Wshadow"
	"-Wformat"
	"-Wpointer-arith"
	"-D_REENTRANT"
	"-D_USE_FAST_MACRO"
	"-Wno-long-long"
	"-Wuninitialized"
	"-D_POSIX_PTHREAD_SEMANTICS"
	"-DHAS_MYSQL_DLL"
	"-DHAS_SQLITE_DLL"
	"-Wno-invalid-source-encoding"
	"-Wno-unused-private-field"
	"-Wno-unused-parameter"
	"-Wno-unused-const-variable"
	"-fexceptions"
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
if(CMAKE_SYSTEM_NAME MATCHES "Android")
	add_definitions("-DANDROID")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
else()
	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

##############################################################################

set(acl_path ${CMAKE_CURRENT_SOURCE_DIR}/../lib_acl)
set(acl_include ${acl_path}/include)
set(proto_path ${CMAKE_CURRENT_SOURCE_DIR}/../lib_protocol)
set(proto_include ${proto_path}/include)
set(mysql_include ${CMAKE_CURRENT_SOURCE_DIR}/../include/mysql)
set(sqlite_include ${CMAKE_CURRENT_SOURCE_DIR}/../include/sqlite)

include_directories(
	${mysql_include}
	${sqlite_include}
	${acl_include}
	${proto_include}
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(src ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(sources
	${src}
	${src}/aliyun
	${src}/beanstalk
	${src}/connpool
	${src}/db
	${src}/disque
	${src}/event
	${src}/hsocket
	${src}/http
	${src}/ipc
	${src}/master
	${src}/memcache
	${src}/mime
	${src}/mime/internal
	${src}/queue
	${src}/redis
	${src}/serialize
	${src}/session
	${src}/smtp
	${src}/stdlib
	${src}/stream lib_src
)

foreach(iter ${sources})
	aux_source_directory(${iter} lib_src)
endforeach()

if (CMAKE_SYSTEM_NAME MATCHES "Android")
	set(CMAKE_SHARED_LINKER_FLAGS "-shared")
else()
	set(lib_output_path ${PROJECT_BINARY_DIR}/lib)

	set(LIBRARY_OUTPUT_PATH ${lib_output_path})
	add_library(_acl_cpp_static STATIC ${lib_src})
	SET_TARGET_PROPERTIES(_acl_cpp_static PROPERTIES OUTPUT_NAME "acl_cpp")

	if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
		set(CMAKE_SHARED_LINKER_FLAGS
			"-shared -Wl,-rpath,. -L${lib_output_path} -lprotocol -lacl -liconv -lz -lpthread")
	else()
		set(CMAKE_SHARED_LINKER_FLAGS
			"-shared -Wl,-rpath,. -L${lib_output_path} -lprotocol -lacl -lz -lpthread")
	endif()

	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})
	link_directories(${lib_output_path})
endif()

if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_library(acl_cpp SHARED ${lib_src})
	target_link_libraries(acl_cpp protocol acl)
endif()

##############################################################################
