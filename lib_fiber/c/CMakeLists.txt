cmake_minimum_required(VERSION 2.8)
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

#message(${PROJECT_SOURCE_DIR})

enable_language(C CXX ASM)

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Android")
	# xxx: -flto can't be used on NDK with gnustl_shared
	if (ANDROID_STL MATCHES "gnustl_shared")
		add_definitions("-Oz -g")
	else()
		add_definitions("-O3 -flto")
	endif()

	add_definitions("-DANDROID")
	add_definitions("-Wno-unused-command-line-argument")
	add_definitions("-fdata-sections -ffunction-sections")
	set(UNIX_OS true)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
	add_definitions("-O2")
	set(UNIX_OS true)
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	add_definitions("-O2")
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

add_definitions(
	"-W"
	"-std=gnu99"
	"-fPIC"
	"-Wall"
#	"-Werror"
	"-Wshadow"
	"-Wpointer-arith"
#	"-Waggregate-return"
	"-Wmissing-prototypes"
	"-D_REENTRANT"
	"-D_USE_FAST_MACRO"
	"-DACL_WRITEABLE_CHECK"
	"-Wno-long-long"
	"-Wuninitialized"
#	"-DUSE_JMP"
	"-DUSE_FAST_RING"
	"-D_POSIX_PTHREAD_SEMANTICS"
	"-DACL_PREPARE_COMPILE"
#	"-Wno-invalid-source-encoding"
	"-Wstrict-prototypes"
#	"-DUSE_JMP_DEF"
	"-DUSE_BOOST_JMP"
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
	add_definitions("-Wno-incompatible-pointer-types-discards-qualifiers")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
else()
	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

# for lower version ndk such add ndk12b, the read/write functions have confusions in the ndk's header,
# so we should disable hook io in io.c
if (ACL_DISABLE_HOOK_IO  MATCHES "YES")
	add_definitions("-DDISABLE_HOOK_IO")
endif()

##############################################################################

set(src ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(sources
	${src}
	${src}/fiber
	${src}/common
	${src}/event
	${src}/hook
	${src}/dns
)

foreach(iter ${sources})
	aux_source_directory(${iter} lib_src)
	message(STATUS ">>add dir ${iter}")
endforeach()

list(APPEND lib_src ${src}/fiber/boost/make_gas.S
	${src}/fiber/boost/jump_gas.S)

if (CMAKE_SYSTEM_NAME MATCHES "Android")
	set(lib_output_path ${CMAKE_CURRENT_SOURCE_DIR}/../../android/lib/${ANDROID_ABI})
else()
	set(lib_output_path ${PROJECT_BINARY_DIR}/lib)
endif()

set(LIBRARY_OUTPUT_PATH ${lib_output_path})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})

add_library(fiber_static STATIC ${lib_src})
SET_TARGET_PROPERTIES(fiber_static PROPERTIES OUTPUT_NAME "fiber")
link_directories(${lib_output_path})

if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND ACL_BUILD_SHARED MATCHES "YES")
	if (CMAKE_SYSTEM_NAME MATCHES "Android")
		if (ANDROID_STL MATCHES "gnustl_shared")
			set(sys_ldflags "-shared -O3")
		else()
			set(sys_ldflags "-shared -O3 -flto")
		endif()
		target_compile_options(fiber_static PRIVATE -fvisibility=hidden)
	elseif (${UNIX_OS})
		set(sys_ldflags "-shared -O3")
		target_compile_options(fiber_static PRIVATE -fvisibility=hidden)
	endif()

	set(CMAKE_SHARED_LINKER_FLAGS "${sys_ldflags}")
	add_library(fiber SHARED ${lib_src})
endif()

##############################################################################
