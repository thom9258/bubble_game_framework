cmake_minimum_required(VERSION 3.2)

project(bubble VERSION 0.1.0)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -ggdb -pg")
set(CMAKE_CXX_STANDARD 23)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(GLSLC_EXECUTABLE glslc REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/bubble.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/resources.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/direct_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/gpu_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/bitmap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/task_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/uniform_arena.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/input.cpp
)

add_library(${PROJECT_NAME} ${SOURCES})

set(COMPILE_AND_EMBED_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/compile_and_embed.py")
set(GENERATED_SHADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_shaders")
file(MAKE_DIRECTORY ${GENERATED_SHADERS_DIR})

function(compile_and_embed_shader SHADER_FILENAME VAR_NAME OUTPUT_HEADER_NAME)
    set(SHADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILENAME}")
    set(HEADER_FILE "${GENERATED_SHADERS_DIR}/${OUTPUT_HEADER_NAME}")
    
    add_custom_command(
        OUTPUT ${HEADER_FILE}
        COMMAND Python3::Interpreter ${COMPILE_AND_EMBED_SCRIPT} 
                ${GLSLC_EXECUTABLE} ${SHADER_PATH} ${HEADER_FILE} ${VAR_NAME}
        DEPENDS ${SHADER_PATH} ${COMPILE_AND_EMBED_SCRIPT}
        COMMENT "Compiles and bakes GLSL '${SHADER_FILENAME}'"
        VERBATIM
    )

    # Fortæl dit IDE og build-system at filen genereres dynamisk
    set_source_files_properties(${HEADER_FILE} PROPERTIES GENERATED TRUE)

    # Markér de genererede filer som nødvendige for at bygge projektet
    target_sources(${PROJECT_NAME} PRIVATE ${HEADER_FILE})
endfunction()

compile_and_embed_shader("shaders/flatcolor.vert" "flatcolor_vert" "flatcolor_vert.hpp")
compile_and_embed_shader("shaders/flatcolor.frag" "flatcolor_frag" "flatcolor_frag.hpp")
compile_and_embed_shader("shaders/texture.vert" "texture_vert" "texture_vert.hpp")
compile_and_embed_shader("shaders/texture.frag" "texture_frag" "texture_frag.hpp")

target_include_directories(${PROJECT_NAME} 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include

    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${Vulkan_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    ${Vulkan_LIBRARIES}
    ${SDL2_LIBRARIES}
    glm::glm
)


