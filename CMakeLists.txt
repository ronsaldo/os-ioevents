# OSIOEvents cmake build script.
cmake_minimum_required(VERSION 2.8)
project(OSIOEvents C)

# Check the build type
if (CMAKE_BUILD_TYPE STREQUAL "")
	# CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
	# differentiation between debug and release builds.
	set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif ()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Output paths.
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/dist")
set(OSIOEvents_DATA_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/dist/data")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/;${CMAKE_MODULE_PATH}")

if(APPLE)
	set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

# Turn warnings and use C99.
if (${CMAKE_C_COMPILER_ID} STREQUAL MSVC)
	# using Visual Studio
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
	if(UNIX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
    endif()

	# Export symbols from applications.
	#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--export-dynamic")
endif()

# Perform platform checks
include(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
include(${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
include(${CMAKE_ROOT}/Modules/CheckLibraryExists.cmake)

# Set output dir.
set(EXECUTABLE_OUTPUT_PATH "${OSIOEvents_BINARY_DIR}/dist")
set(LIBRARY_OUTPUT_PATH "${OSIOEvents_BINARY_DIR}/dist")

# Add the current directory.
include_directories(
.
"${PROJECT_SOURCE_DIR}/include"
)

# Build the library
add_subdirectory(lib)
