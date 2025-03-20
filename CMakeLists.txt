cmake_minimum_required(VERSION 3.10)

project(ProjetCompression)

set(CMAKE_CXX_STANDARD 17)

# Fetch SDL3 from source and build it with the project
include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG main  # You can change the tag to a specific version if needed
)

# Ensure SDL3 is built as a static library
set(SDL3_BUILD_STATIC_LIBS ON CACHE BOOL "Build SDL3 as a static library" FORCE)

# Make SDL3 available
FetchContent_MakeAvailable(SDL3)

# ImGui configuration
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/libs/imgui")
set(IMGUI_BACKENDS_DIR "${IMGUI_DIR}/backends")

# Include directories for SDL3 and ImGui
include_directories(
    "${CMAKE_SOURCE_DIR}/include"
    "${SDL3_SOURCE_DIR}/include"  # SDL3 includes after fetching it
    "${IMGUI_DIR}"
    "${IMGUI_BACKENDS_DIR}"
)

# ImGui sources
file(GLOB IMGUI_SOURCES "${IMGUI_DIR}/*.cpp")
file(GLOB IMGUI_BACKEND_SOURCES "${IMGUI_BACKENDS_DIR}/imgui_impl_sdl3.cpp" "${IMGUI_BACKENDS_DIR}/imgui_impl_sdlrenderer3.cpp")

# Project source files
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES} ${IMGUI_BACKEND_SOURCES})

# Link SDL3 statically
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    SDL3::SDL3  # Automatically links to the SDL3 target provided by FetchContent
)
