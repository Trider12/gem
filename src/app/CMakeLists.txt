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

target_include_directories(${TARGET} PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${DIR_THIRDPARTY}/SDL/include
	${DIR_THIRDPARTY}/imgui
)

# ==================================================================================================
# Libraries
# ==================================================================================================

if (WIN32)
	target_link_directories(${TARGET} PRIVATE
		${DIR_THIRDPARTY}/SDL/lib/${PLATFORM}/${ARCH}
	)
elseif (LINUX)
	# TODO
endif()

find_package(OpenGL REQUIRED)

target_link_libraries(${TARGET} PRIVATE
	SDL2
	SDL2main
	SDL2test
	${OPENGL_gl_LIBRARY}
)

# ==================================================================================================
# Custom commands
# ==================================================================================================

if(CMAKE_BUILD_TYPE MATCHES Debug)
	file(COPY ${DIR_THIRDPARTY}/SDL/bin/${PLATFORM}/${ARCH}/ DESTINATION ${DIR_EXPORT}/${PLATFORM}/${ARCH}/Debug)
else()
	file(COPY ${DIR_THIRDPARTY}/SDL/bin/${PLATFORM}/${ARCH}/ DESTINATION ${DIR_EXPORT}/${PLATFORM}/${ARCH}/Release)
endif()