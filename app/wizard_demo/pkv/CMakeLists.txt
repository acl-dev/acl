cmake_minimum_required(VERSION 2.8)
project(pkv)

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
    message(STATUS "build pkv for release version")
elseif (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
	message(STATUS "build pkv for debug version")
else()
    message(STATUS "build pkv for default version")
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
set(acl_path ${home_path}/lib_acl)
set(acl_cpp_path ${home_path}/lib_acl_cpp)
set(fiber_path ${home_path}/lib_fiber/c)
set(fiber_cpp_path ${home_path}/lib_fiber/cpp)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${acl_path}/include
    ${acl_cpp_path}/include
    ${fiber_path}/include
    ${fiber_cpp_path}/include
)

set(base_path ${CMAKE_CURRENT_SOURCE_DIR})
set(src_paths
    ${base_path}
    ${base_path}/test
    ${base_path}/coder
    ${base_path}/action
    ${base_path}/dao
    ${base_path}/db
    ${base_path}/db/rocksdb
    ${base_path}/db/wt
)

foreach(iter ${src_paths})
    aux_source_directory(${iter} src_files)
endforeach()

##############################################################################

#SET(CMAKE_VERBOSE_MAKEFILE on)

add_definitions(
    "-g"
    "-W"
    "-O3"
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
)

if (${BUILD_WITH_C11} MATCHES "YES")
    add_definitions("-std=c++11")
else()
    add_definitions("-std=c++17")
endif()

#find_library(acl_lib acl_all PATHS /usr/lib /usr/local/lib)
#find_library(fiber_cpp_lib fiber_cpp PATHS /usr/lib /usr/local/lib)
#find_library(fiber_lib fiber PATHS /usr/lib /usr/local/lib)

set(acl_all ${home_path}/lib_fiber/lib/libfiber_cpp.a ${home_path}/libacl_all.a)
set(fiber ${home_path}/lib_fiber/lib/libfiber.a)

find_library(yyjson NAMES yyjson PATHS /usr/lib /usr/local/lib)
set(lib_all ${acl_all} ${yyjson})

if(${HAS_ROCKSDB} MATCHES "YES")
    find_library(rocksdb NAMES rocksdb PATHS /usr/lib /usr/local/lib)
    add_definitions("-DHAS_ROCKSDB")
    set(lib_all ${lib_all} ${rocksdb})
endif()

if(${HAS_WT} MATCHES "YES")
    find_library(wiredtiger NAMES wiredtiger PATHS /usr/lib /usr/local/lib)
    add_definitions("-DHAS_WT")
    set(lib_all ${lib_all} ${wiredtiger})
endif()

if(${HAS_JEMALLOC} MATCHES "YES")
    find_library(jemalloc NAMES jemalloc PATHS /usr/lib /usr/local/lib)
    add_definitions("-DHAS_JEAMALLOC")
    set(lib_all ${lib_all} ${jemalloc})
endif()

#set(wt -lwiredtiger)
#set(uring -luring-ffi)
#set(iconv -liconv)

set(lib_all ${lib_all} ${fiber})

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(lib_all ${lib_all} -liconv)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(lib_all ${lib_all} ${uring} ${iconv} ${jemalloc})
endif()

set(lib_all ${lib_all} -lz -lpthread -ldl)

set(output_path ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${output_path})
link_directories(${output_path})

add_executable(pkv ${src_files} test/test_coder.cpp test/test_coder.h test/test_db.cpp test/test_db.h)
target_link_libraries(pkv ${lib_all})

###############################################################################
