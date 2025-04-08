cmake_minimum_required(VERSION 3.16)
project(RaytracerCG1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define sources
file(GLOB_RECURSE SRC_FILES
    src/*.cpp
    src/*.h
)

# Define executable
add_executable(${PROJECT_NAME} ${SRC_FILES})

# stb_image
target_include_directories(${PROJECT_NAME} PRIVATE external/stb)

# SDL2
if (WIN32)
    # Use local SDL2 from external/SDL2
    set(SDL2_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/SDL2/include")
    set(SDL2_LIB_DIR "${CMAKE_SOURCE_DIR}/external/SDL2/lib")

    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIR})

    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${SDL2_LIB_DIR}/SDL2.lib
        ${SDL2_LIB_DIR}/SDL2main.lib
        ${SDL2_LIB_DIR}/SDL2_ttf.lib
    )

    # Copy SDL2 DLLs after build
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_LIB_DIR}/SDL2.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_LIB_DIR}/SDL2_ttf.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying SDL2 DLLs to output directory..."
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS SDL_MAIN_HANDLED)
else()
    find_package(SDL2 REQUIRED)
    find_path(SDL2_TTF_INCLUDE_DIR SDL_ttf.h PATH_SUFFIXES SDL2)
    find_library(SDL2_TTF_LIBRARY NAMES SDL2_ttf sdl2_ttf)

    if (SDL2_TTF_INCLUDE_DIR AND SDL2_TTF_LIBRARY)
        target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_TTF_INCLUDE_DIR})
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 ${SDL2_TTF_LIBRARY})
    else()
        message(FATAL_ERROR "SDL2_ttf not found. Please install the sdl2_ttf package.")
    endif()

    target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_MAIN_HANDLED)
endif()

# Enable OpenMP
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
endif()

# ImGui
file(GLOB IMGUI_SRC
    external/imgui/*.cpp
    external/imgui/backends/imgui_impl_sdl2.cpp
    external/imgui/backends/imgui_impl_sdlrenderer2.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE external/imgui)
target_include_directories(${PROJECT_NAME} PRIVATE external/imgui/backends)

target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_SRC})

# Include all source subdirectories
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/core
    src/geometry
    src/material
    src/modelling
    src/platform
    src/renderer
    src/scene
)

# === Copy assets/ folder to the executable output directory ===
set(ASSETS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")
set(ASSETS_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets")

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ASSETS_SOURCE_DIR}" "${ASSETS_TARGET_DIR}"
    COMMENT "Copying assets to output directory..."
)
