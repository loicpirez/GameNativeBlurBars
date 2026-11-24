cmake_minimum_required(VERSION 3.22.1)

Project(Pluvia)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wno-unused-function -Wimplicit-function-declaration")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -s")

#add_subdirectory(proot)
option(GAMENATIVE_VULKAN_ONLY "Build the Vulkan compositor without unrelated native modules" OFF)
if(NOT GAMENATIVE_VULKAN_ONLY)
add_subdirectory(virglrenderer)
add_subdirectory(patchelf)
endif()
add_subdirectory(lsfg-vk-android/thirdparty/dxbc EXCLUDE_FROM_ALL)
add_subdirectory(extras/adrenotools)

if(NOT GAMENATIVE_VULKAN_ONLY)
add_library(winlator SHARED
            winlator/drawable.c
            winlator/sysvshared_memory.c
            winlator/xconnector_epoll.c
            winlator/alsa_client.c
            winlator/patchelf_wrapper.cpp)

target_link_libraries(winlator
                      log
                      android
                      jnigraphics
                      aaudio
                      EGL
                      GLESv2
                      GLESv3)

target_link_options(winlator PRIVATE -Wl,-z,max-page-size=16384)
endif()

find_program(GLSLANG_VALIDATOR NAMES glslangValidator glslang REQUIRED)


add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/window_vert.h
    COMMAND ${GLSLANG_VALIDATOR} -V --vn window_vert_code -x
            -o ${CMAKE_CURRENT_BINARY_DIR}/window_vert.h
            ${CMAKE_CURRENT_SOURCE_DIR}/winlator/window.vert
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/winlator/window.vert
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/window_frag.h
    COMMAND ${GLSLANG_VALIDATOR} -V --vn window_frag_code -x
            -o ${CMAKE_CURRENT_BINARY_DIR}/window_frag.h
            ${CMAKE_CURRENT_SOURCE_DIR}/winlator/window.frag
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/winlator/window.frag
)

foreach(stage vert frag)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/blur_bars_${stage}.h
        COMMAND ${GLSLANG_VALIDATOR} -V --vn blur_bars_${stage}_code -x
                -o ${CMAKE_CURRENT_BINARY_DIR}/blur_bars_${stage}.h
                ${CMAKE_CURRENT_SOURCE_DIR}/winlator/blur_bars.${stage}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/winlator/blur_bars.${stage})
endforeach()

add_library(vulkan_renderer SHARED
            winlator/BlurBarsVulkan.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/blur_bars_vert.h
            ${CMAKE_CURRENT_BINARY_DIR}/blur_bars_frag.h
            winlator/vulkan_jni.cpp
            winlator/VulkanRendererContext.cpp
            winlator/VulkanRendererScanout.cpp
            winlator/VulkanRendererLsfg.cpp
            lsfg/vk_dispatch.c
            lsfg/lsfg_dll.c
            lsfg/lsfg_dxbc.cpp
            lsfg/lsfg_probe.c
            lsfg/lsfg_jni.c
            lsfg/lsfg_common.cpp
            lsfg/lsfg_shaders.cpp
            lsfg/lsfg_mipmaps.cpp
            lsfg/lsfg_generate.cpp
            lsfg/lsfg_alpha.cpp
            lsfg/lsfg_beta.cpp
            lsfg/lsfg_gamma.cpp
            lsfg/lsfg_delta.cpp
            lsfg/lsfg_chain.cpp
            lsfg/lsfg_pacer.cpp
            lsfg/vkr_lsfg.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/window_vert.h
            ${CMAKE_CURRENT_BINARY_DIR}/window_frag.h)

target_include_directories(vulkan_renderer PRIVATE
                           ${CMAKE_CURRENT_BINARY_DIR}
                           ${CMAKE_CURRENT_SOURCE_DIR}/winlator
                           ${CMAKE_CURRENT_SOURCE_DIR}/lsfg)

set_target_properties(vulkan_renderer PROPERTIES
                      CXX_STANDARD 17
                      CXX_STANDARD_REQUIRED ON)

target_compile_options(vulkan_renderer PRIVATE
                       -Wall -Wextra -fvisibility=hidden)

target_link_libraries(vulkan_renderer
                      log
                      android
                      vulkan
                      adrenotools
                      dxbc
                      dl
                      atomic)

target_link_options(vulkan_renderer PRIVATE -Wl,-z,max-page-size=16384)
