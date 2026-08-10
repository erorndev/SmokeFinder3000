cmake_minimum_required(VERSION 3.25)

project(SmokeFinder3000 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT MSVC)
    message(FATAL_ERROR "Please use a compiler that supports the MSVC ABI.")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/utf-8)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()


# Dependencies & DLL
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui)
set(LIMBO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/limbo)
set(MINHOOK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/minhook)
add_subdirectory(${MINHOOK_DIR} ${CMAKE_BINARY_DIR}/minhook-build)

if(MSVC)
    set_target_properties(minhook PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()

add_library(SmokeFinder3000 SHARED
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp

    src/Main.cpp
    src/Dll.cpp
    src/gui/Gui.cpp
)

# Include
target_include_directories(SmokeFinder3000 PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${LIMBO_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(SmokeFinder3000 PRIVATE
    minhook
    d3d11
    dxgi
)