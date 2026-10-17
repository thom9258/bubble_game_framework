cmake_minimum_required(VERSION 3.2)

project(game VERSION 0.1.0)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -ggdb -pg")
set(CMAKE_CXX_STANDARD 23)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)

set(SOURCES
    ../source/bubble.cpp
    ../source/context.cpp
    ../source/renderer.cpp
    ../source/timer.cpp
    ../source/resources.cpp
    ../source/direct_buffer.cpp
    ../source/gpu_buffer.cpp
    ../source/texture.cpp
    ../source/bitmap.cpp
    ../source/task_graph.cpp
    ../source/uniform_arena.cpp
    ../source/input.cpp
)

add_executable(${PROJECT_NAME} main.cpp ${SOURCES})

target_include_directories(${PROJECT_NAME} 
    PRIVATE
    ../include
    ${Vulkan_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    ${Vulkan_LIBRARIES}
    ${SDL2_LIBRARIES}
    glm::glm
)
