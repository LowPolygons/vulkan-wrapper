cmake_minimum_required(VERSION 3.28)

project(vulkan_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(SHADER_SLANG_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/conways_game_of_life.slang
)
message(STATUS "Shader sources: ${SHADER_SLANG_SOURCES}")

find_program(SLANGC_EXECUTABLE slangc REQUIRED)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

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
    src/glfw/glfw_window_handler.cc
    src/device/helpers.cc
    src/device/device_and_queue_container.cc
    src/instance_and_surface/instance_and_surface_container.cc
    src/swapchain/swapchain_info_container.cc
    src/buffers/buffer_copy.cc
    src/buffers/data_buffer_container.cc
    src/shaders/shader_utils.cc
    src/pipeline/graphics_pipeline_container.cc
    src/buffers/command_buffer_container.cc
    src/buffers/command_buffer_container.cc
    src/buffers/transition_buffer_layout.cc
    src/syncs/sync_object_container.cc
    src/wrapper_boilerplate.cc
    src/mandelbulb_app.cc
    src/3d_app.cc
    src/conways_app.cc
    src/pipeline/compute_pipeline_container.cc
    src/debugger/debugger_container.cc
    src/main.cpp
)


add_slang_shader_target( shaders SOURCES ${SHADER_SLANG_SOURCES})
add_dependencies(vulkan_app 
    shaders
)

target_link_libraries(vulkan_app PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
)

target_compile_definitions(vulkan_app PRIVATE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    # IMPERATIVE if you want to be able to resize without crashing
    VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
)
