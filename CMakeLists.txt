cmake_minimum_required(VERSION 3.10)

project(ProjetCompression)

set(CMAKE_CXX_STANDARD 17)

set(SDL3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libs/SDL3/include")
set(SDL3_LIB_DIR "${CMAKE_SOURCE_DIR}/libs/SDL3")

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/libs/imgui")
set(IMGUI_BACKENDS_DIR "${IMGUI_DIR}/backends")

include_directories(
    "${CMAKE_SOURCE_DIR}/include"
    "${SDL3_INCLUDE_DIR}"
    "${IMGUI_DIR}"
    "${IMGUI_BACKENDS_DIR}"
)

file(GLOB IMGUI_SOURCES "${IMGUI_DIR}/*.cpp")

file(GLOB IMGUI_BACKEND_SOURCES "${IMGUI_BACKENDS_DIR}/imgui_impl_sdl3.cpp" "${IMGUI_BACKENDS_DIR}/imgui_impl_sdlrenderer3.cpp")

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES} ${IMGUI_BACKEND_SOURCES})

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    "${SDL3_LIB_DIR}/libSDL3.a"
)


