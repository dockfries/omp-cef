find_package(miniz CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)

if (NOT TARGET tiny-aes)
    add_subdirectory(${CMAKE_SOURCE_DIR}/deps/tiny-aes ${CMAKE_BINARY_DIR}/_deps_tinyaes)
endif()

add_subdirectory(common)

if (DEV_ALL_TARGETS OR BUILD_SERVER_OMP)
    if (NOT TARGET OMP-SDK)
        add_subdirectory(${CMAKE_SOURCE_DIR}/deps/omp-sdk ${CMAKE_BINARY_DIR}/_deps_omp_sdk)
    endif()
    add_subdirectory(omp)
endif()

# SAMP (x86 and x64)
if (DEV_ALL_TARGETS OR BUILD_SERVER_SAMP)
    if (NOT TARGET sampgdk)
        # sampgdk is consumed as a git submodule; build it static and full
        # (its IDL native implementations and module-init chain are required),
        # matching the plugin's word size.
        set(SAMPGDK_STATIC ON CACHE BOOL "" FORCE)
        set(SAMPGDK_TINY OFF CACHE BOOL "" FORCE)
        set(SAMPGDK_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(SAMPGDK_BUILD_PLUGINS OFF CACHE BOOL "" FORCE)
        if (IS_32BIT)
            set(SAMPGDK_ARCH "32" CACHE STRING "" FORCE)
        else()
            set(SAMPGDK_ARCH "64" CACHE STRING "" FORCE)
        endif()
        set(SAMP_SDK_ROOT "${CMAKE_SOURCE_DIR}/deps/samp-plugin-sdk")
        add_subdirectory(${CMAKE_SOURCE_DIR}/deps/sampgdk ${CMAKE_BINARY_DIR}/_deps_sampgdk)
    endif()
    add_subdirectory(samp)
endif()