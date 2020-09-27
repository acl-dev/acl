cmake_minimum_required(VERSION 2.8)

#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

#message(project_dir=${PROJECT_SOURCE_DIR})

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Android")
	# xxx: -flto can't be used on NDK with gnustl_shared
	if (ANDROID_STL MATCHES "gnustl_shared")
		add_definitions("-Oz -g -DHAVE_NO_ATEXIT")
	else()
		add_definitions("-O3 -flto")
		if (HAS_ATOMIC MATCHES "YES")
			add_definitions("-DHAS_ATOMIC")
		endif()
	endif()
	add_definitions("-DANDROID")
	add_definitions("-DACL_CLIENT_ONLY")
#	add_definitions("-Wno-invalid-source-encoding")
	add_definitions("-fdata-sections -ffunction-sections")
	add_definitions("-Wno-unused-command-line-argument")
	string(APPEND CMAKE_C_FLAGS "-Qunused-arguments")
	set(UNIX_OS true)
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
	add_definitions("-O2")
	set(UNIX_OS true)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
	add_definitions("-O3")
	add_definitions("-Wno-invalid-source-encoding")
	add_definitions("-fdata-sections -ffunction-sections")
	add_definitions("-flto")
	add_definitions("-Os")
	set(UNIX_OS true)
elseif (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	add_definitions("-O2")
	set(UNIX_OS true)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(CMAKE_SHARED_LINKER_FLAGS "/DYNAMICBASE ws2_32.lib IPHlpApi.lib")
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

#if (CMAKE_BUILD_TYPE STREQUAL "")
#	set(CMAKE_BUILD_TYPE "DEBUG")
#else()
#	string(TOUPPER CMAKE_BUILD_TYPE CMAKE_BUILD_TYPE)
#endif()

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

	if (CMAKE_COMPILER_IS_GNUCC)
		execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
			OUTPUT_VARIABLE GCC_VERSION)
		string(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
		list(GET GCC_VERSION_COMPONENTS 0 GCC_MAJOR)
		list(GET GCC_VERSION_COMPONENTS 1 GCC_MINOR)

		message(STATUS "cmake version=${CMAKE_VERSION}")

		set(GCC_VERSION "${GCC_MAJOR}.${GCC_MINOR}")
		message(STATUS "gcc version=${GCC_VERSION}")

		if (GCC_VERSION GREATER "4.7")
			message(STATUS "gcc version major=${GCC_MAJOR}")
			message(STATUS "gcc version minor=${GCC_MINOR}")
			add_definitions("-Wno-implicit-fallthrough")
		endif()
	endif()

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

if (ACL_CLIENT_ONLY MATCHES "YES")
	add_definitions("-DACL_CLIENT_ONLY")
	message(STATUS "acl: ACL_CLIENT_ONLY been set")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Android")
	set(lib_output_path ${CMAKE_CURRENT_SOURCE_DIR}/../android/lib/${ANDROID_ABI})
else()
	set(lib_output_path ${PROJECT_BINARY_DIR}/lib)
endif()

set(LIBRARY_OUTPUT_PATH ${lib_output_path})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})

add_library(acl_static STATIC ${lib_src})
SET_TARGET_PROPERTIES(acl_static PROPERTIES OUTPUT_NAME "acl")
link_directories(${lib_output_path})

if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND ACL_BUILD_SHARED MATCHES "YES")
	if (CMAKE_SYSTEM_NAME MATCHES "Android")
		if (ANDROID_STL MATCHES "gnustl_shared")
			set(sys_ldflags "-shared")
		else()
			set(sys_ldflags "-shared -flto")
		endif()
		target_compile_options(acl_static PRIVATE -fvisibility=hidden)
	elseif (${UNIX_OS})
		set(sys_ldflags "-shared -lpthread -ldl")
		target_compile_options(acl_static PRIVATE -fvisibility=hidden)
	endif()

	set(CMAKE_SHARED_LINKER_FLAGS "${sys_ldflags}")
	add_library(acl SHARED ${lib_src})
endif()

##############################################################################
