cmake_minimum_required(VERSION 3.15)

set(TARGET gem)
project(${TARGET})

# ==================================================================================================
# Variables
# ==================================================================================================

set(DIR_THIRDPARTY ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set(DIR_EXPORT     ${CMAKE_CURRENT_SOURCE_DIR}/build)

# ==================================================================================================
# OS specific
# ==================================================================================================

if (UNIX AND NOT APPLE AND NOT ANDROID AND NOT WEBGL)
	set(LINUX TRUE)
endif()

if (WIN32)
	set(PLATFORM windows)
elseif(LINUX)
	set(PLATFORM linux)
endif()

set(ARCH x64)

if(CMAKE_BUILD_TYPE MATCHES Debug)
	set(CONFIG "Debug")
else()
    set(CONFIG "Release")
endif()

# ==================================================================================================
# Compiling
# ==================================================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (WIN32)
    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        add_compile_options(
            /Zc:__cplusplus
            /W4
            /WX
            /external:I ${DIR_THIRDPARTY}
            /external:W0
            $<$<NOT:$<CONFIG:DEBUG>>:/O2>
        )
        add_link_options($<$<NOT:$<CONFIG:DEBUG>>:/SUBSYSTEM:WINDOWS>)
    endif()
elseif(LINUX)
	add_compile_options(
        -Wall
        -Werror
        -Wno-error=unused-function
        -fstrict-aliasing
        -isystem ${DIR_THIRDPARTY}
        $<$<NOT:$<CONFIG:DEBUG>>:-O3>
        $<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>
        -pthread
    )
    add_link_options(
        -stdlib=libc++
        -pthread
    )

	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")
endif()

# ==================================================================================================
# Sub-projects
# ==================================================================================================

add_subdirectory(src/app)