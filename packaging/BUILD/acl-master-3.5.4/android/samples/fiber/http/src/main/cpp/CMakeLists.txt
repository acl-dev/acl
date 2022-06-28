# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    message(FATAL_ERROR "Please into another dir to build!")
endif()

add_definitions(
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
        "-fPIC"
)

if (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "DEBUG")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
    set(build_rpath "release")
    add_definitions("-g")
    add_definitions("-DACL_CPP_LOG_SKIP_FILE")
    add_definitions("-Os")
    #	add_definitions("-O2")
elseif (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
    set(build_rpath "debug")
    add_definitions("-g")
    add_definitions("-Os")
else()
    message(FATAL_ERROR "debug")
    add_definitions("-g")
    set(build_rpath "debug")
endif()

#string(TOUPPER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME)
if(CMAKE_SYSTEM_NAME MATCHES "Android")
    add_definitions("-DANDROID -Wno-unused-command-line-argument")
    list(APPEND ANDROID_LINKER_FLAGS -Qunused-arguments)
    string( APPEND CMAKE_CXX_FLAGS " -Qunused-arguments")

    # xxx: don't set -flto for shared mode in cflags
    #add_definitions("-flto")

    # xxx: don't set any CMAKE_SHARED_LINKER_FLAGS on NDK with gnustl_shared
    if (NOT ANDROID_STL MATCHES "gnustl_shared")
        set(CMAKE_SHARED_LINKER_FLAGS "-fvisibility=hidden")
    endif()
    add_definitions("-fvisibility=hidden")
    add_definitions("-fvisibility-inlines-hidden")
    add_definitions("-fdata-sections -ffunction-sections")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    add_definitions(
            "-Wno-invalid-source-encoding"
            "-Wno-unused-private-field"
            "-Wno-unused-const-variable"
    )
else()
    message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

##############################################################################

set(acl_home ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../..)
set(acl_c_path ${acl_home}/lib_acl)
set(acl_c_include ${acl_c_path}/include)
set(acl_cpp_path ${acl_home}/lib_acl_cpp)
set(acl_cpp_include ${acl_cpp_path}/include)
set(fiber_path ${acl_home}/lib_fiber)
set(fiber_c_include ${fiber_path}/c/include)
set(fiber_cpp_include ${fiber_path}/cpp/include)

include_directories(
        "./"
        ${acl_c_include}
        ${acl_cpp_include}
        ${fiber_c_include}
        ${fiber_cpp_include}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set(src ${CMAKE_CURRENT_SOURCE_DIR})
set(sources
        ${src}
)

foreach(iter ${sources})
    aux_source_directory(${iter} lib_src)
endforeach()

if(CMAKE_SYSTEM_NAME MATCHES "Android")
    if (NOT ANDROID_STL MATCHES "gnustl_shared")
        set(CMAKE_SHARED_LINKER_FLAGS "-shared -O3 -flto")
    endif()
endif()

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

if(CMAKE_SYSTEM_NAME MATCHES "Android")
    set(lib_acl_path ${acl_home}/android/lib/${ANDROID_ABI})
    set(libacl ${lib_acl_path}/libacl.a)
    set(libacl_cpp ${lib_acl_path}/libacl_cpp.a)
    set(libprotocol ${lib_acl_path}/libprotocol.a)
    set(libfiber ${lib_acl_path}/libfiber.a)
    set(libfiber_cpp ${lib_acl_path}/libfiber_cpp.a)

    set(lib_all ${libfiber_cpp} ${libacl_cpp} ${libprotocol} ${libacl} ${libfiber} -lz)
#    set(lib_all ${libacl_cpp} ${libprotocol} ${libacl} -lz)
endif()

add_library(native-lib SHARED ${lib_src})
target_link_libraries(native-lib ${lib_all} ${log-lib})

###############################################################################

