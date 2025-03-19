# Specify the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(ProjetCompression)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)

# Define paths for the libraries
set(SDL3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libs/SDL3/include")
set(SDL3_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/SDL3")

# ImGui path
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/libs/imgui")
set(IMGUI_BACKENDS_DIR "${IMGUI_DIR}/backends")

# Add include directories
include_directories(
    "${CMAKE_SOURCE_DIR}/include"        # Your project's header files
    "${SDL3_INCLUDE_DIR}"                # SDL3 headers
    "${IMGUI_DIR}"                       # ImGui headers
    "${IMGUI_BACKENDS_DIR}"              # ImGui backend headers (including ImGui SDL3 renderer)
)

# Add ImGui source files
file(GLOB IMGUI_SOURCES "${IMGUI_DIR}/*.cpp")

# Add ImGui backend source files for SDL3 and SDL Renderer
file(GLOB IMGUI_BACKEND_SOURCES "${IMGUI_BACKENDS_DIR}/imgui_impl_sdl3.cpp" "${IMGUI_BACKENDS_DIR}/imgui_impl_sdlrenderer3.cpp")

# Add the source files from your project
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

# Add the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES} ${IMGUI_BACKEND_SOURCES})

# Link SDL3 and ImGui libraries
target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    "${SDL3_LIB_DIR}/libSDL3.a"         # Link to the SDL3 static library
)

# Optionally, if SDL3 requires platform-specific libraries (e.g., Cocoa on macOS), link them here
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa")
endif()

# If you're using SDL3 dynamically, you might also need to link SDL3 dynamically
# target_link_libraries(${PROJECT_NAME} PRIVATE SDL3)
