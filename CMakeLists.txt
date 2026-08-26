cmake_minimum_required(VERSION 3.28)

project(vulkan_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Vulkan Wrapper Library
add_library(vulkan_wrapper
    vulkan_wrapper/glfw/glfw_window_handler.cc
    vulkan_wrapper/device/helpers.cc
    vulkan_wrapper/device/device_and_queue_container.cc
    vulkan_wrapper/instance_and_surface/instance_and_surface_container.cc
    vulkan_wrapper/swapchain/swapchain_info_container.cc
    vulkan_wrapper/buffers/buffer_copy.cc
    vulkan_wrapper/buffers/data_buffer_container.cc
    vulkan_wrapper/shaders/shader_utils.cc
    vulkan_wrapper/pipeline/graphics_pipeline_container.cc
    vulkan_wrapper/buffers/command_buffer_container.cc
    vulkan_wrapper/buffers/command_buffer_container.cc
    vulkan_wrapper/buffers/transition_buffer_layout.cc
    vulkan_wrapper/syncs/sync_object_container.cc
    vulkan_wrapper/pipeline/compute_pipeline_container.cc
    vulkan_wrapper/debugger/debugger_container.cc
    vulkan_wrapper/wrapper_boilerplate.cc
)

target_include_directories(vulkan_wrapper PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(vulkan_wrapper PUBLIC
    Vulkan::Vulkan
    glfw
    glm::glm
)


target_compile_definitions(vulkan_wrapper PUBLIC
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
)

find_program(SLANGC_EXECUTABLE slangc REQUIRED)
set(SHADER_SLANG_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apps/conways/conways_game_of_life.slang
)
message(STATUS "Shader sources: ${SHADER_SLANG_SOURCES}")

function (add_slang_shader_target TARGET)
    # thank god this was written for me by khronos because what is going on here
    cmake_parse_arguments ("SHADER" "" "" "SOURCES" ${ARGN})
    set (SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR}/shaders)
    set (ENTRY_POINTS -entry vertMain -entry fragMain -entry compMain)
    add_custom_command (
        OUTPUT ${SHADERS_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
    )
  add_custom_command (
          OUTPUT  ${SHADERS_DIR}/conways.spv
          COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o conways.spv
          WORKING_DIRECTORY ${SHADERS_DIR}
          DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
          COMMENT "Compiling Slang Shaders"
          VERBATIM
  )
  add_custom_target (${TARGET} DEPENDS ${SHADERS_DIR}/conways.spv)
endfunction()

add_executable(vulkan_app
    # src/mandelbulb_app.cc
    # src/3d_app.cc
    src/apps/conways/conways_app.cc
    src/main.cpp
)

add_slang_shader_target( shaders SOURCES ${SHADER_SLANG_SOURCES})

add_dependencies(vulkan_app 
    shaders
)

target_link_libraries(vulkan_app PRIVATE
    vulkan_wrapper
)

target_include_directories(vulkan_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
