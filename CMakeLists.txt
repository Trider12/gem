cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tools/vcpkg/scripts/buildsystems/vcpkg.cmake)

set(TARGET gem)
project(${TARGET})

# ==================================================================================================
# Variables
# ==================================================================================================

set(DIR_THIRDPARTY ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set(DIR_ASSETS     ${CMAKE_CURRENT_SOURCE_DIR}/assets)

# ==================================================================================================
# OS specific
# ==================================================================================================

if(UNIX AND NOT ANDROID AND NOT WEBGL)
	if(APPLE)
		if(NOT IOS)
			set(DARWIN TRUE)
		endif()
	else()
		set(LINUX TRUE)
	endif()
endif()

if(WIN32)
	set(PLATFORM windows)
	set(ARCH x64)
elseif(LINUX)
	set(PLATFORM linux)
	set(ARCH x64)
elseif(DARWIN)
	set(PLATFORM darwin)
	set(ARCH x64)
elseif(ANDROID)
	set(PLATFORM android)
	set(ARCH ${ANDROID_ABI})
elseif(IOS)
	set(PLATFORM ios)
	set(ARCH ${ARCHS})
endif()

set(DIR_EXPORT ${CMAKE_CURRENT_SOURCE_DIR}/build/${PLATFORM}/${ARCH}/${CMAKE_BUILD_TYPE})

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
			$<$<NOT:$<CONFIG:DEBUG>>:/WX>
			/external:I ${DIR_THIRDPARTY}
			/external:W0
			$<$<NOT:$<CONFIG:DEBUG>>:/O2>
		)

		add_link_options(
			$<$<NOT:$<CONFIG:DEBUG>>:/SUBSYSTEM:WINDOWS>
		)
	endif()
else()
	add_compile_options(
		-Wall
		$<$<NOT:$<CONFIG:DEBUG>>:-Werror>
		-Wno-error=unused-function
		-fstrict-aliasing
		-isystem ${DIR_THIRDPARTY}
		$<$<NOT:$<CONFIG:DEBUG>>:-O3>
	)
endif()

if(LINUX)
	add_compile_options(
		$<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>
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