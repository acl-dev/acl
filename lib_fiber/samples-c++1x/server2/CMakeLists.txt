cmake_minimum_required(VERSION 2.8)
project(server)

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
    message(STATUS "build server for release version")
elseif (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
	message(STATUS "build server for debug version")
else()
    message(STATUS "build server for default version")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	add_definitions("-Wno-invalid-source-encoding")
    include_directories("/usr/local/include")
    SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    SET(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
else()
	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

##############################################################################

set(home_path ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(acl_cpp_inc ${home_path}/lib_acl_cpp/include)
set(fiber_cpp_inc ${home_path}/lib_fiber/cpp/include)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${acl_cpp_inc}
    ${fiber_cpp_inc}
)

set(base_path ${CMAKE_CURRENT_SOURCE_DIR})
set(src_paths ${base_path})

foreach(iter ${src_paths})
    aux_source_directory(${iter} src_files)
endforeach()

##############################################################################

#SET(CMAKE_VERBOSE_MAKEFILE on)

add_definitions(
    "-g"
    "-W"
    "-Wall"
    "-Werror"
    "-Wshadow"
    "-Wformat"
    "-Wpointer-arith"
    "-D_REENTRANT"
    "-Wno-long-long"
    "-Wuninitialized"
    "-D_POSIX_PTHREAD_SEMANTICS"
    "-fexceptions"
    "-Wno-unused-parameter"
    "-Wno-error=deprecated-declarations"
    "-Wno-deprecated-declarations"
    "-fPIC"
    "-O3"
    "-std=c++11"
)

set(libacl_all ${home_path}/libacl_all.a)
set(libfiber ${home_path}/lib_fiber/lib/libfiber.a)
set(libfiber_cpp ${home_path}/lib_fiber/lib/libfiber_cpp.a)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(lib_all  ${libfiber_cpp} ${libacl_all} ${libfiber} -liconv -lz -lpthread -ldl)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(lib_all  ${libfiber_cpp} ${libacl_all} ${libfiber} -lz -lpthread -ldl)
endif()

set(output_path ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${output_path})
link_directories(${output_path})

add_executable(server ${src_files})
target_link_libraries(server ${lib_all})

###############################################################################
