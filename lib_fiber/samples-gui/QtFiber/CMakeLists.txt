cmake_minimum_required(VERSION 3.5)

project(QtFiber LANGUAGES CXX)

#set(CMAKE_BUILD_TYPE Release)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

set(base_path ${CMAKE_CURRENT_SOURCE_DIR})
set(src_paths .)
foreach(iter ${src_paths})
    aux_source_directory(${iter} src_files)
endforeach()

add_executable(QtFiber ${src_files} mainwindow.ui)

set(acl_path ../../..)

include_directories(
    ${acl_path}/lib_acl/include
    ${acl_path}/lib_acl_cpp/include
    ${acl_path}/lib_fiber/c/include
    ${acl_path}/lib_fiber/cpp/include
)

add_definitions("-DACL_DLL"
    "-DACL_CPP_DLL"
    "-DHTTP_DLL"
    "-DICMP_DLL"
    "-DSMTP_DLL"
    "-DFIBER_CPP_DLL"
    "-D_CRT_SECURE_NO_WARNINGS"
    "-D_WINSOCK_DEPRECATED_NO_WARNINGS"
)

if (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
    set(acl_libs_path ${CMAKE_CURRENT_SOURCE_DIR}/../../../x64/ReleaseDll)
else()
    set(acl_libs_path ${CMAKE_CURRENT_SOURCE_DIR}/../../../x64/DebugDll)
endif()

set(lib_all ${acl_libs_path}/libfiber_cpp.lib
    ${acl_libs_path}/lib_acl_cpp.lib
    ${acl_libs_path}/lib_protocol.lib
    ${acl_libs_path}/lib_acl.lib
    ${acl_libs_path}/libfiber.lib)

target_link_libraries(QtFiber PRIVATE Qt5::Widgets ${lib_all} Ws2_32)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${acl_libs_path}/libfiber_cpp.dll"
        "${acl_libs_path}/libfiber.dll"
        "${acl_libs_path}/lib_acl_cpp.dll"
        "${acl_libs_path}/lib_acl.dll"
        "${acl_libs_path}/lib_protocol.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${QT_BINDIR})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Running windeployqt..."
    COMMAND ${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Deploying dependencies using windeployqt"
)
