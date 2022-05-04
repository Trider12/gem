cmake_minimum_required(VERSION 3.15)

set(TARGET app)
project(${TARGET})

# ==================================================================================================
# Sources
# ==================================================================================================

file(GLOB HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)

file(GLOB SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
	${DIR_THIRDPARTY}/imgui/*.cpp
	${DIR_THIRDPARTY}/imgui/backends/imgui_impl_opengl3.cpp
	${DIR_THIRDPARTY}/imgui/backends/imgui_impl_sdl.cpp
	${DIR_THIRDPARTY}/imgui/misc/cpp/imgui_stdlib.cpp
)

# ==================================================================================================
# Target
# ================================================================================================== 

add_executable(${TARGET} ${HEADERS} ${SOURCES})
set_target_properties(${TARGET} PROPERTIES 
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DIR_EXPORT}/${PLATFORM}/${ARCH}/Release
	RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${DIR_EXPORT}/${PLATFORM}/${ARCH}/Debug
)

# ==================================================================================================
# Includes
# ==================================================================================================

target_include_directories(${TARGET} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${DIR_THIRDPARTY}/imgui
	${DIR_THIRDPARTY}/imgui/backends
	${DIR_THIRDPARTY}/imgui/misc/cpp
	${DIR_THIRDPARTY}/IconFontCppHeaders
	${DIR_THIRDPARTY}/asio/asio/include
)

target_compile_definitions(${TARGET} PRIVATE 
	ASIO_NO_DEPRECATED 
	ASIO_NO_TS_EXECUTORS
	_WIN32_WINNT=0x0601
)

# ==================================================================================================
# Libraries
# ==================================================================================================

find_package(OpenGL REQUIRED)
find_package(OpenSSL REQUIRED)

target_link_libraries(${TARGET} PRIVATE
	${OPENGL_LIBRARIES}
	OpenSSL::SSL
	OpenSSL::Crypto
)

if (WIN32)
	target_link_directories(${TARGET} PRIVATE
		${DIR_THIRDPARTY}/SDL/lib/${PLATFORM}/${ARCH}
	)
	target_include_directories(${TARGET} PRIVATE
		${DIR_THIRDPARTY}/SDL/include
	)
	target_link_libraries(${TARGET} PRIVATE
		SDL2
		SDL2main
		SDL2test
	)
elseif (LINUX)
	find_package(SDL2 REQUIRED)
	target_include_directories(${TARGET} PRIVATE
		${SDL2_INCLUDE_DIRS}
	)
	target_link_libraries(${TARGET} PRIVATE
		${SDL2_LIBRARIES}
		${CMAKE_DL_LIBS}
	)
endif()

# ==================================================================================================
# Custom commands
# ==================================================================================================

if(CMAKE_BUILD_TYPE MATCHES Debug)
	file(COPY ${DIR_THIRDPARTY}/SDL/bin/${PLATFORM}/${ARCH}/ DESTINATION ${DIR_EXPORT}/${PLATFORM}/${ARCH}/Debug)
else()
	file(COPY ${DIR_THIRDPARTY}/SDL/bin/${PLATFORM}/${ARCH}/ DESTINATION ${DIR_EXPORT}/${PLATFORM}/${ARCH}/Release)
endif()