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
	${DIR_THIRDPARTY}/imgui/backends/imgui_impl_sdl2.cpp
	${DIR_THIRDPARTY}/imgui/misc/cpp/imgui_stdlib.cpp
	${DIR_THIRDPARTY}/imgui/misc/freetype/imgui_freetype.cpp
	${DIR_THIRDPARTY}/stb/stb_image.c
)

if(WIN32)
	list(APPEND SOURCES ${DIR_THIRDPARTY}/nativefiledialog-extended/src/nfd_win.cpp)
elseif(LINUX)
	list(APPEND SOURCES ${DIR_THIRDPARTY}/nativefiledialog-extended/src/nfd_portal.cpp)
elseif(DARWIN)
	list(APPEND SOURCES ${DIR_THIRDPARTY}/nativefiledialog-extended/src/nfd_cocoa.m)
endif()

# ==================================================================================================
# Target
# ================================================================================================== 

add_executable(${TARGET} ${SOURCES})

set_target_properties(${TARGET} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${DIR_EXPORT}
)

# ==================================================================================================
# Preprocessor
# ==================================================================================================

target_compile_definitions(${TARGET} PRIVATE
	_WIN32_WINNT=0x0601
	ASIO_NO_DEPRECATED 
	ASIO_NO_TS_EXECUTORS
	IMGUI_ENABLE_FREETYPE
	IMGUI_USE_WCHAR32
	RAPIDJSON_HAS_STDSTRING
	STBI_NO_STDIO
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
	${DIR_THIRDPARTY}/stb
	${DIR_THIRDPARTY}/nativefiledialog-extended/src/include
)

# ==================================================================================================
# Libraries
# ==================================================================================================

add_library(SSL STATIC IMPORTED)
add_library(Crypto STATIC IMPORTED)

if(WIN32)
	set_target_properties(SSL PROPERTIES
		IMPORTED_LOCATION ${DIR_THIRDPARTY}/openssl/lib/${PLATFORM}/${ARCH}/libssl.lib
	)
	set_target_properties(Crypto PROPERTIES
		IMPORTED_LOCATION ${DIR_THIRDPARTY}/openssl/lib/${PLATFORM}/${ARCH}/libcrypto.lib
	)
	target_link_libraries(${TARGET} PRIVATE crypt32.lib) # provided by OS
else()
	set_target_properties(SSL PROPERTIES
		IMPORTED_LOCATION ${DIR_THIRDPARTY}/openssl/lib/${PLATFORM}/${ARCH}/libssl.a
	)
	set_target_properties(Crypto PROPERTIES
		IMPORTED_LOCATION ${DIR_THIRDPARTY}/openssl/lib/${PLATFORM}/${ARCH}/libcrypto.a
	)
endif()

if(LINUX)
	find_package(PkgConfig REQUIRED) # provided by OS
	pkg_check_modules(DBUS REQUIRED dbus-1)
	message("Using DBUS version: ${DBUS_VERSION}")
	target_compile_definitions(${TARGET} PRIVATE NFD_PORTAL)
	target_include_directories(${TARGET} PRIVATE ${DBUS_INCLUDE_DIRS})
	target_link_libraries(${TARGET} PRIVATE ${DBUS_LIBRARIES})
elseif(DARWIN)
	find_library(APPKIT_LIBRARY AppKit) # provided by OS
	target_link_libraries(${TARGET} PRIVATE ${APPKIT_LIBRARY})
endif()

find_package(OpenGL REQUIRED) # provided by OS
find_package(SDL2 REQUIRED) # provided by vcpkg
find_package(Freetype REQUIRED) # provided by vcpkg

target_include_directories(${TARGET} PRIVATE
	${SDL2_INCLUDE_DIRS}
	${FREETYPE_INCLUDE_DIRS}
)

if(WIN32)
	target_link_libraries(${TARGET} PRIVATE
		SDL2::SDL2
		SDL2::SDL2main
	)
else()
	target_link_libraries(${TARGET} PRIVATE
		SDL2::SDL2-static
		SDL2::SDL2main
	)
endif()

target_link_libraries(${TARGET} PRIVATE
	SSL
	Crypto
	${OPENGL_LIBRARIES}
	${FREETYPE_LIBRARIES}
	${CMAKE_DL_LIBS}
)

# ==================================================================================================
# Custom commands
# ==================================================================================================

file(COPY ${DIR_ASSETS} DESTINATION ${DIR_EXPORT})
