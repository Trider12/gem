cmake_minimum_required(VERSION 3.15)

set(TARGET app)
project(${TARGET})

set(USE_SDLRENDERER TRUE)

# ==================================================================================================
# Sources
# ==================================================================================================

file(GLOB SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
	${DIR_THIRDPARTY}/imgui/*.cpp
	${DIR_THIRDPARTY}/imgui/backends/imgui_impl_sdl.cpp
	${DIR_THIRDPARTY}/imgui/misc/cpp/imgui_stdlib.cpp
	${DIR_THIRDPARTY}/imgui/misc/freetype/imgui_freetype.cpp
	${DIR_THIRDPARTY}/stb/stb_image.c
)

if(USE_SDLRENDERER)
	list(APPEND SOURCES ${DIR_THIRDPARTY}/imgui/backends/imgui_impl_sdlrenderer.cpp)
else()
	list(APPEND SOURCES ${DIR_THIRDPARTY}/imgui/backends/imgui_impl_opengl3.cpp)
endif()

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

if(ANDROID)
	add_library(${TARGET} SHARED ${SOURCES})

	set_target_properties(${TARGET} PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY ${DIR_EXPORT}
	)
else()
	add_executable(${TARGET} ${SOURCES})

	set_target_properties(${TARGET} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${DIR_EXPORT}
	)
endif()

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

if(ANDROID)
	target_compile_definitions(${TARGET} PRIVATE
		IMGUI_IMPL_OPENGL_ES3
	)
endif()

if(USE_SDLRENDERER)
	target_compile_definitions(${TARGET} PRIVATE
		GEM_SDLRENDERER
	)
endif()

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

find_package(OpenSSL REQUIRED) # provided by vcpkg
find_package(SDL2 REQUIRED) # provided by vcpkg
find_package(Freetype REQUIRED) # provided by vcpkg

target_include_directories(${TARGET} PRIVATE
	${OPENSSL_INCLUDE_DIR}
	${SDL2_INCLUDE_DIRS}
	${FREETYPE_INCLUDE_DIRS}
)

if(WIN32 OR ANDROID)
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

if(ANDROID)
	target_link_libraries(${TARGET} PRIVATE
		EGL
		GLESv3
	)
else()
	find_package(OpenGL REQUIRED) # provided by OS
	target_link_libraries(${TARGET} PRIVATE
		${OPENGL_LIBRARIES}
	)
endif()

target_link_libraries(${TARGET} PRIVATE
	${OPENSSL_LIBRARIES}
	${FREETYPE_LIBRARIES}
	${CMAKE_DL_LIBS}
)

# ==================================================================================================
# Custom commands
# ==================================================================================================

file(COPY ${DIR_ASSETS} DESTINATION ${DIR_EXPORT})
