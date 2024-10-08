cmake_minimum_required(VERSION 3.2.0)
#cmake_minimum_required(VERSION 2.8.0)
project(acl_cpp)

#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

#string(TOUPPER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME)
if(CMAKE_SYSTEM_NAME MATCHES "Android")
	# xxx: -flto can't be used on NDK with gnustl_shared
	if (ANDROID_STL MATCHES "gnustl_shared")
		add_definitions("-Oz -g -DHAVE_NO_ATEXIT")
	else()
		add_definitions("-O3 -flto")
	endif()

	add_definitions("-DANDROID")
	add_definitions("-DACL_CPP_LOG_SKIP_FILE")
	add_definitions("-Wno-unused-command-line-argument")
#	add_definitions("-Wno-invalid-source-encoding")
	add_definitions("-fdata-sections -ffunction-sections")
	string(APPEND CMAKE_CXX_FLAGS "-Qunused-arguments")
	set(UNIX_OS true)
elseif (CMAKE_SYSTEM_NAME MATCHES "OHOS")
	add_definitions("-O3 -flto")
	add_definitions("-DANDROID")
	add_definitions("-DACL_CPP_LOG_SKIP_FILE")
	add_definitions("-fdata-sections -ffunction-sections")
	add_definitions("-Wno-unused-command-line-argument")
	add_definitions("-Wno-c99-extensions")
	string(APPEND CMAKE_CXX_FLAGS "-Qunused-arguments")
	set(UNIX_OS true)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
#	add_definitions("-Wno-invalid-source-encoding")
	add_definitions("-O2")
	set(UNIX_OS true)
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
#	add_definitions("-O2 -DUSE_SYS_ICONV")
	add_definitions("-O2")
	include_directories(
		/usr/include
		/usr/local/include
	)
	set(UNIX_OS true)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
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
	message(STATUS "acl_cpp: ACL_CLIENT_ONLY been set")
	add_definitions("-DACL_CLIENT_ONLY")
endif()

if (HAS_MBEDTLS MATCHES "YES")
	message(STATUS "acl_cpp: HAS_MBEDTLS been set")
	add_definitions("-DHAS_MBEDTLS")
endif()

#if (ACL_HOOK_NEW MATCHES "YES")
#	message(STATUS "acl_cpp: ACL_HOOK_NEW been set")
#	add_definitions("-DACL_HOOK_NEW")
#endif()

##############################################################################

set(acl_path ${CMAKE_CURRENT_SOURCE_DIR}/../lib_acl)
set(acl_include ${acl_path}/include)
set(proto_path ${CMAKE_CURRENT_SOURCE_DIR}/../lib_protocol)
set(proto_include ${proto_path}/include)
set(mysql_include ${CMAKE_CURRENT_SOURCE_DIR}/../include/mysql)
set(pgsql_include ${CMAKE_CURRENT_SOURCE_DIR}/../include/pgsql)
set(sqlite_include ${CMAKE_CURRENT_SOURCE_DIR}/../include/sqlite)
set(polarssl_include ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(openssl_include ${CMAKE_CURRENT_SOURCE_DIR}/../include/openssl-1.1.1q)

include_directories(
	${mysql_include}
	${pgsql_include}
	${sqlite_include}
	${polarssl_include}
	${openssl_include}
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
	${src}/mqtt
	${src}/net
	${src}/queue
	${src}/redis
	${src}/serialize
	${src}/session
	${src}/smtp
	${src}/stdlib
	${src}/stream lib_src
)

foreach(iter ${sources})
	if (ACL_BUILD_VERBOSE MATCHES "YES")
		message(STATUS ">>add dir ${iter}")
	endif()
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
		add_definitions("-Wno-unused-private-field"
			"-Wno-unused-const-variable"
		)
	endif()

	if (!CMAKE_BUILD_TYPE STREQUAL "RELEASE")
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

	if (HAS_MBEDTLS MATCHES "YES")
		add_definitions("-DHAS_MBEDTLS")
		message(STATUS "HAS_MBEDTLS been set, mbedtls enable!")
	elseif (HAS_MBEDTLS_DLL MATCHES "YES")
		add_definitions("-DHAS_MBEDTLS_DLL")
		message(STATUS "HAS_MBEDTLS_DLL been set, mbedtls enable!")
	else()
		message(STATUS "mbedtls disable!")
	endif()

	if (HAS_OPENSSL MATCHES "YES")
		add_definitions("-DHAS_OPENSSL")
		message(STATUS "HAS_OPENSSL been set, openssl enable!")
	elseif (HAS_OPENSSL_DLL MATCHES "YES")
		add_definitions("-DHAS_OPENSSL_DLL")
		message(STATUS "HAS_OPENSSL_DLL been set, openssl enable!")
	else()
		message(STATUS "openssl disable!")
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
		"-DHAS_MBEDTLS_DLL"
		"-DUSE_WIN_ICONV")
	if (ACL_BUILD_SHARED MATCHES "YES")
		add_definitions("-DACL_CPP_DLL -DACL_CPP_EXPORTS"
			"-DACL_DLL -DHTTP_DLL -DICMP_DLL -DSMTP_DLL"
		)
	endif()
	aux_source_directory(${src}/stdlib/internal lib_src)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Android")
	set(lib_output_path ${CMAKE_CURRENT_SOURCE_DIR}/../android/lib/${ANDROID_ABI})
elseif (CMAKE_SYSTEM_NAME MATCHES "OHOS")
	set(lib_output_path ${CMAKE_CURRENT_SOURCE_DIR}/../harmony/lib/${OHOS_ARCH})
else()
	set(lib_output_path ${PROJECT_BINARY_DIR}/../lib)
endif()

set(LIBRARY_OUTPUT_PATH ${lib_output_path})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})

add_library(acl_cpp_static STATIC ${lib_src})

if (${WIN_OS})
	set_target_properties(acl_cpp_static PROPERTIES
		OUTPUT_NAME "libacl_cpp"
		ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}/static
		LIBRARY_OUTPUT_DIRECTORY ${lib_output_path}/static
	)
else()
	set_target_properties(acl_cpp_static PROPERTIES OUTPUT_NAME "acl_cpp")
endif()

link_directories(${lib_output_path})

if(CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "Darwin")
	install(TARGETS acl_cpp_static
		EXPORT acl_cpp_static-targets
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	)
endif()

if (CMAKE_INSTALL_INCLUDEDIR MATCHES "")
	set(CMAKE_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/include)
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/acl_cpp/
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/acl-lib/acl_cpp/"
)

#if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND ACL_BUILD_SHARED MATCHES "YES")
if (ACL_BUILD_SHARED MATCHES "YES")
	if (CMAKE_SYSTEM_NAME MATCHES "Android")
		if (ANDROID_STL MATCHES "gnustl_shared")
			set(sys_ldflags "-shared -lz")
		else()
			set(sys_ldflags "-shared -flto -lz")
		endif()
		target_compile_options(acl_cpp_static PRIVATE
			-fvisibility=hidden -fvisibility-inlines-hidden)
	elseif (CMAKE_SYSTEM_NAME MATCHES "OHOS")
		set(sys_ldflags "-shared -flto -lz")
		target_compile_options(acl_cpp_static PRIVATE
				-fvisibility=hidden -fvisibility-inlines-hidden)
	elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
		set(sys_ldflags "-shared -lz -liconv -lpthread -ldl")
		target_compile_options(acl_cpp_static PRIVATE
			-fvisibility=hidden -fvisibility-inlines-hidden)
		set(CMAKE_MACOSX_RPATH 1)
	elseif (${UNIX_OS})
		set(sys_ldflags "-shared -lz -lpthread -ldl")
		target_compile_options(acl_cpp_static PRIVATE
			-fvisibility=hidden -fvisibility-inlines-hidden)
	endif()

	if (${WIN_OS})
		link_directories(${lib_output_path}/shared)
	endif()

	add_library(acl_cpp_shared SHARED ${lib_src})

	if (${WIN_OS})
		set_target_properties(acl_cpp_shared PROPERTIES
			OUTPUT_NAME "libacl_cpp"
			RUNTIME_OUTPUT_DIRECTORY ${lib_output_path}/shared
			ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}/shared
			LIBRARY_OUTPUT_DIRECTORY ${lib_output_path}/shared
		)
		target_link_libraries(acl_cpp_shared libprotocol libacl)
	else()
		set(lc_flags "-Wl,-rpath,${CMAKE_INSTALL_PREFIX}/lib -L${lib_output_path}")
		set(CMAKE_SHARED_LINKER_FLAGS "${lc_flags} ${sys_ldflags}")
		set_target_properties(acl_cpp_shared PROPERTIES OUTPUT_NAME "acl_cpp")
		target_link_libraries(acl_cpp_shared protocol acl)
	endif()

	if(CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "Darwin")
		install(TARGETS acl_cpp_shared
			EXPORT acl_cpp_shared-targets
			LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		)
	endif()
endif()

##############################################################################
