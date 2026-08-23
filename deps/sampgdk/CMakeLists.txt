cmake_minimum_required(VERSION 3.10)
project(sampgdk C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/deps/cmake-modules")

set(SAMP_SDK_ROOT "${CMAKE_SOURCE_DIR}/deps/samp-plugin-sdk")
find_package(SAMPSDK REQUIRED)
include_directories(${SAMPSDK_DIR})

set(SAMPGDK_SRC sampgdk.c)
set(SAMPGDK_HEADERS sampgdk.h)

add_library(sampgdk STATIC ${SAMPGDK_SRC} ${SAMPGDK_HEADERS})

# sampgdk is linked into shared plugin modules (cef.so). Non-PIC objects
# cannot be linked into a shared library on x86-64, so build PIC on ELF.
if(UNIX AND NOT APPLE)
    set_property(TARGET sampgdk PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

target_include_directories(sampgdk PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SAMPSDK_DIR}/amx
)

if (MSVC)
    target_compile_definitions(sampgdk PRIVATE SAMPGDK_WINDOWS=1)
else()
    target_compile_definitions(sampgdk PRIVATE SAMPGDK_LINUX=1)
endif()