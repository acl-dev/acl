cmake_minimum_required(VERSION 2.8)
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

#message(${PROJECT_SOURCE_DIR})

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
if(CMAKE_SYSTEM_NAME MATCHES "Android")
	add_definitions("-DANDROID")
	add_definitions("-Wno-incompatible-pointer-types-discards-qualifiers")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
else()
	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

##############################################################################

set(src ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(acl_path ${CMAKE_CURRENT_SOURCE_DIR}/../../lib_acl)
set(acl_include ${acl_path}/include)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${acl_include}
	${acl_path}/src/master
)

set(sources
	${src}
	${src}/master
)

foreach(iter ${sources})
	aux_source_directory(${iter} lib_src)
endforeach()

if(NOT CMAKE_SYSTEM_NAME MATCHES "ANDROID")
	set(lib_output_path ${PROJECT_BINARY_DIR}/lib)

	set(LIBRARY_OUTPUT_PATH ${lib_output_path})
	add_library(fiber_static STATIC ${lib_src})
	SET_TARGET_PROPERTIES(fiber_static PROPERTIES OUTPUT_NAME "fiber")

	set(CMAKE_SHARED_LINKER_FLAGS
		-Wl,-rpath,. -L${lib_output_path} -lacl)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})
	link_directories(${lib_output_path})
endif()

if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_library(fiber SHARED ${lib_src})
	target_link_libraries(fiber acl)
endif()

##############################################################################
