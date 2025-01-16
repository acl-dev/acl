# the minimum version of CMake.
cmake_minimum_required(VERSION 3.4.1)
project(acl_one)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

set(acl_home ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../..)
set(acl_c_path ${acl_home}/lib_acl)
set(acl_c_include ${acl_c_path}/include)
set(pro_c_path ${acl_home}/lib_protocol)
set(pro_c_include ${acl_home}/lib_protocol/include)
set(acl_cpp_path ${acl_home}/lib_acl_cpp)
set(acl_cpp_include ${acl_cpp_path}/include)

include_directories(
    ${acl_c_include}
    ${pro_c_include}
    ${acl_cpp_include}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_definitions("-fvisibility=hidden -fvisibility-inlines-hidden")
add_definitions("-Os")
add_definitions("-DNDEBUG")
add_definitions("-fvisibility=hidden")
add_definitions("-fvisibility-inlines-hidden")
add_definitions("-fdata-sections -ffunction-sections")

set(mysrc ${CMAKE_CURRENT_SOURCE_DIR})
set(mysources
    ${mysrc}
)

foreach (iter ${mysources})
    aux_source_directory(${iter} mylib_src)
endforeach()

add_library(acl SHARED ${mylib_src})
#add_library(acl SHARED hello.cpp)

add_subdirectory(${acl_c_path} acl)
add_subdirectory(${pro_c_path} protocol)
add_subdirectory(${acl_cpp_path} acl_cpp)

set(lib_acl_path ${acl_home}/harmony/lib/${OHOS_ARCH})
set(libacl ${lib_acl_path}/libacl.a)
set(libacl_cpp ${lib_acl_path}/libacl_cpp.a)
set(libprotocol ${lib_acl_path}/libprotocol.a)

set(lib_all ${libacl_cpp} ${libprotocol} ${libacl} -lz)
set(CMAKE_SHARED_LINKER_FLAGS "-shared -O3 -flto")

find_library(
    hilog-lib
    hilog_ndk.z
)
target_link_libraries(acl PUBLIC libace_napi.z.so ${lib_all} ${hilog-lib} libace_napi.z.so libc++.a)
#target_link_libraries(acl PUBLIC libace_napi.z.so ${lib_all} ${hilog-lib} libc++.a)
