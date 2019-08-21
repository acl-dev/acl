cmake_minimum_required(VERSION 2.8)
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

#string(TOUPPER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME)
if(CMAKE_SYSTEM_NAME MATCHES "Android")
	add_definitions("-DANDROID")
	set(UNIX_OS true)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(UNIX_OS true)
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	set(UNIX_OS true)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(UNIX_OS true)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(WIN_OS true)
else()
	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

##############################################################################

set(acl_path ${CMAKE_CURRENT_SOURCE_DIR}/../lib_acl)
set(acl_include ${acl_path}/include)
set(proto_path ${CMAKE_CURRENT_SOURCE_DIR}/../lib_protocol)
set(proto_include ${proto_path}/include)
set(zlib_include ${CMAKE_CURRENT_SOURCE_DIR}/../include/zlib)
set(mysql_include ${CMAKE_CURRENT_SOURCE_DIR}/../include/mysql)
set(pgsql_include ${CMAKE_CURRENT_SOURCE_DIR}/../include/pgsql)
set(sqlite_include ${CMAKE_CURRENT_SOURCE_DIR}/../include/sqlite)
set(polarssl_include ${CMAKE_CURRENT_SOURCE_DIR}/../include)

include_directories(#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
	${zlib_include}
	${mysql_include}
	${pgsql_include}
	${sqlite_include}
	${polarssl_include}
	${acl_include}
	${proto_include}
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

#if (${WIN_OS})
#	set(iconv_include ${CMAKE_CURRENT_SOURCE_DIR}/src/stdlib/internal)
#	include_directories(iconv_include)
#endif()

set(src ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(sources
	${src}
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

##############################################################################

if (${UNIX_OS})
	add_definitions(
		"-W"
		"-O2"
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
		"-DACL_PREPARE_COMPILE"
		"-Wno-unused-parameter"
		"-fexceptions"
		)

	if (!CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
		add_definitions("-Wno-invalid-source-encoding"
			"-Wno-unused-private-field"
			"-Wno-unused-const-variable"
			)
	endif()

	if (CMAKE_BUILD_TYPE STREQUAL "")
		set(CMAKE_BUILD_TYPE "DEBUG")
	else()
		string(TOUPPER CMAKE_BUILD_TYPE CMAKE_BUILD_TYPE)
	endif()

	if (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
		add_definitions("-O2")
	else()
		add_definitions("-g")
	endif()

	if (DISABLE_DB MATCHES "YES")
		add_definitions("-DACL_DB_DISABLE")
		message(STATUS "DB (mysql, pgsql, sqlite) was disabled!")
	else()
		if (HAS_MYSQL MATCHES "YES")
			add_definitions("-DHAS_MYSQL")
			message(STATUS "HAS_MYSQL been set, mysql enable!")
		elseif (HAS_MYSQL_DLL MATCHES "YES")
			add_definitions("-DHAS_MYSQL_DLL")
			message(STATUS "HAS_MYSQL_DLL been set, mysql enable!")
		else()
			message(STATUS "mysql disable!")
		endif()

		if (HAS_PGSQL MATCHES "YES")
			add_definitions("-DHAS_PGSQL")
			message(STATUS "HAS_PGSQL been set, pgsql enable!")
		elseif (HAS_PGSQL_DLL MATCHES "YES")
			add_definitions("-DHAS_PGSQL_DLL")
			message(STATUS "HAS_PGSQL_DLL been set, pgsql enable!")
		else()
			message(STATUS "pgsql disable!")
		endif()

		if (HAS_SQLITE MATCHES "YES")
			add_definitions("-DHAS_SQLITE")
			message(STATUS "HAS_SQLITE been set, sqlite enable!")
		elseif (HAS_SQLITE_DLL MATCHES "YES")
			add_definitions("-DHAS_SQLITE_DLL")
			message(STATUS "HAS_SQLITE_DLL been set, sqlite enable!")
		else()
			message(STATUS "sqlite disable!")
		endif()
	endif()

	if (HAS_POLARSSL MATCHES "YES")
		add_definitions("-DHAS_POLARSSL")
		message(STATUS "HAS_POLARSSL been set, polarssl enable!")
	elseif (HAS_POLARSSL_DLL MATCHES "YES")
		add_definitions("-DHAS_POLARSSL_DLL")
		message(STATUS "HAS_POLARSSL_DLL been set, polarssl enable!")
	else()
		message(STATUS "polarssl disable!")
	endif()


elseif (${WIN_OS})
	add_definitions(
		"-Ycacl_stdafx.hpp"
		"-DACL_WRITEABLE_CHECK"
		"-DACL_PREPARE_COMPILE"
		"-DHAS_ZLIB_DLL"
		"-DHAS_MYSQL_DLL"
		"-DHAS_SQLITE_DLL"
		"-DHAS_PGSQL_DLL"
		"-DHAS_POLARSSL_DLL"
		"-DUSE_WIN_ICONV")
	aux_source_directory(${src}/stdlib/internal lib_src)
endif()

if (ACL_CLIENT_ONLY MATCHES "YES")
	add_definitions("-DACL_CLIENT_ONLY")
	message(STATUS "acl_cpp: ACL_CLIENT_ONLY been set")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Android")
	set(CMAKE_SHARED_LINKER_FLAGS "-shared -lz")
	set(CMAKE_SHARED_LINKER_FLAGS "-lz")
	add_definitions("-DACL_CLIENT_ONLY")
	add_definitions("-Wno-invalid-source-encoding")

	set(lib_output_path ${PROJECT_BINARY_DIR}/lib)
	set(LIBRARY_OUTPUT_PATH ${lib_output_path})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})

	add_library(acl_cpp_static STATIC ${lib_src})
	SET_TARGET_PROPERTIES(acl_cpp_static PROPERTIES OUTPUT_NAME "acl_cpp")
	link_directories(${lib_output_path})
else()
	set(lib_output_path ${PROJECT_BINARY_DIR}/lib)

	set(LIBRARY_OUTPUT_PATH ${lib_output_path})
	add_library(acl_cpp_static STATIC ${lib_src})
	SET_TARGET_PROPERTIES(acl_cpp_static PROPERTIES OUTPUT_NAME "acl_cpp")

	if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
		set(CMAKE_SHARED_LINKER_FLAGS
			"-shared -Wl,-rpath,. -L${lib_output_path} -lprotocol -lacl -liconv -lz -lpthread")
	elseif (${UNIX_OS})
		set(CMAKE_SHARED_LINKER_FLAGS
			"-shared -Wl,-rpath,. -L${lib_output_path} -lprotocol -lacl -lz -lpthread")
	elseif (${WIN_OS})
		set(CMAKE_SHARED_LINKER_FLAGS "/DYNAMICBASE ws2_32.lib")
	endif()

	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})
	link_directories(${lib_output_path})
endif()

if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_library(acl_cpp SHARED ${lib_src})
	target_link_libraries(acl_cpp protocol acl)
endif()

##############################################################################
