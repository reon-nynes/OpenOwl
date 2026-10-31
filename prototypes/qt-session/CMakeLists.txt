cmake_minimum_required(VERSION 3.24)
project(OpenOwlQtSessionSpike LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Network Quick QuickControls2)
set(SPOUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../OwlNativePrototype/vendor/spout/SPOUTSDK" CACHE PATH "Existing pinned Spout checkout for this local experiment")
add_library(spout_spike STATIC
    ${SPOUT_DIR}/SpoutDirectX/SpoutDX/SpoutDX.cpp
    ${SPOUT_DIR}/SpoutGL/SpoutCopy.cpp ${SPOUT_DIR}/SpoutGL/SpoutDirectX.cpp
    ${SPOUT_DIR}/SpoutGL/SpoutFrameCount.cpp ${SPOUT_DIR}/SpoutGL/SpoutSenderNames.cpp
    ${SPOUT_DIR}/SpoutGL/SpoutSharedMemory.cpp ${SPOUT_DIR}/SpoutGL/SpoutUtils.cpp)
target_compile_definitions(spout_spike PUBLIC SPOUT_BUILD_STATIC NOMINMAX)
target_include_directories(spout_spike PUBLIC ${SPOUT_DIR}/SpoutDirectX/SpoutDX ${SPOUT_DIR}/SpoutGL)
target_link_libraries(spout_spike PUBLIC d3d11 dxgi d3dcompiler winmm version shell32 opengl32)
qt_add_executable(owl-session-spike WIN32 engine.cpp)
target_link_libraries(owl-session-spike PRIVATE Qt6::Core Qt6::Gui Qt6::Network spout_spike)
qt_add_executable(owl-debug-spike WIN32 debug.cpp)
target_link_libraries(owl-debug-spike PRIVATE Qt6::Core Qt6::Gui Qt6::Network Qt6::Quick Qt6::QuickControls2)
qt_add_resources(owl-debug-spike "debug_qml" PREFIX "/" FILES Main.qml)
qt_add_executable(owl-control-spike cli.cpp)
target_link_libraries(owl-control-spike PRIVATE Qt6::Core Qt6::Network)
foreach(target owl-session-spike owl-debug-spike owl-control-spike)
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
endforeach()
