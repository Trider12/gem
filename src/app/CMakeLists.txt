cmake_minimum_required(VERSION 3.15)

set(TARGET app)
project(${TARGET})

# ==================================================================================================
# Sources
# ==================================================================================================

file(GLOB SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
	${DIR_THIRDPARTY}/imgui/*.cpp
	${DIR_THIRDPARTY}/imgui/backends/imgui_impl_opengl3.cpp
	${DIR_THIRDPARTY}/imgui/backends/imgui_impl_sdl.cpp
	${DIR_THIRDPARTY}/imgui/misc/cpp/imgui_stdlib.cpp
	${DIR_THIRDPARTY}/imgui/misc/freetype/imgui_freetype.cpp
)

# ==================================================================================================
# Target
# ================================================================================================== 

add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES 
	RUNTIME_OUTPUT_DIRECTORY ${DIR_EXPORT}
)

# ==================================================================================================
# Includes
# ==================================================================================================

target_include_directories(${TARGET} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${DIR_THIRDPARTY}/imgui
	${DIR_THIRDPARTY}/imgui/backends
	${DIR_THIRDPARTY}/imgui/misc/cpp
	${DIR_THIRDPARTY}/imgui/misc/freetype
	${DIR_THIRDPARTY}/IconFontCppHeaders
	${DIR_THIRDPARTY}/asio/asio/include
	${DIR_THIRDPARTY}/openssl/include
	${DIR_THIRDPARTY}/rapidjson/include
)

target_compile_definitions(${TARGET} PRIVATE 
	ASIO_NO_DEPRECATED 
	ASIO_NO_TS_EXECUTORS
	_WIN32_WINNT=0x0601
	IMGUI_ENABLE_FREETYPE
	IMGUI_USE_WCHAR32
	RAPIDJSON_HAS_STDSTRING
)

# ==================================================================================================
# Libraries
# ==================================================================================================

add_library(SSL STATIC IMPORTED)
add_library(Crypto STATIC IMPORTED)

if (WIN32)
	set_target_properties(SSL PROPERTIES 
		IMPORTED_LOCATION ${DIR_THIRDPARTY}/openssl/lib/${PLATFORM}/${ARCH}/libssl.lib
	)
	set_target_properties(Crypto PROPERTIES 
		IMPORTED_LOCATION ${DIR_THIRDPARTY}/openssl/lib/${PLATFORM}/${ARCH}/libcrypto.lib
	)
	target_link_libraries(${TARGET} PRIVATE crypt32.lib)
else()
	set_target_properties(SSL PROPERTIES 
		IMPORTED_LOCATION ${DIR_THIRDPARTY}/openssl/lib/${PLATFORM}/${ARCH}/libssl.a
	)
	set_target_properties(Crypto PROPERTIES 
		IMPORTED_LOCATION ${DIR_THIRDPARTY}/openssl/lib/${PLATFORM}/${ARCH}/libcrypto.a
	)
endif()

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(Freetype REQUIRED)

target_include_directories(${TARGET} PRIVATE
	${SDL2_INCLUDE_DIRS}
	${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(${TARGET} PRIVATE
	SSL
	Crypto
	${OPENGL_LIBRARIES}
	${SDL2_LIBRARIES}
	${FREETYPE_LIBRARIES}
	${CMAKE_DL_LIBS}
)

# ==================================================================================================
# Custom commands
# ==================================================================================================

file(COPY ${DIR_ASSETS} DESTINATION ${DIR_EXPORT})
