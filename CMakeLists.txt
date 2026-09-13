cmake_minimum_required(VERSION 3.24)
project(ZeroVirtualConsole LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
  message(FATAL_ERROR "ZERO Virtual Console currently targets Windows 11 x64 only.")
endif()

add_library(ZeroSdk STATIC
  sdk/ZeroSdk.cpp
)
target_include_directories(ZeroSdk PUBLIC sdk)
target_compile_definitions(ZeroSdk PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

add_executable(ZeroVirtualConsole WIN32
  src/main.cpp
  src/App.cpp
  src/GameRegistry.cpp
  src/RuntimeSession.cpp
  src/RuntimeIpcServer.cpp
  src/RuntimeV3.cpp
  src/RuntimeV4.cpp
  src/ResumeStore.cpp
  src/PlatformStateStore.cpp
  src/AchievementStore.cpp
  src/Settings.cpp
  src/Input.cpp
)

target_include_directories(ZeroVirtualConsole PRIVATE include)
target_compile_definitions(ZeroVirtualConsole PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroVirtualConsole PRIVATE d2d1 dwrite windowscodecs xinput9_1_0 shlwapi shell32 ole32 bcrypt)

add_executable(ZeroTestGame sample_game/TestGame.cpp)
target_include_directories(ZeroTestGame PRIVATE sdk)
target_compile_definitions(ZeroTestGame PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroTestGame PRIVATE ZeroSdk)

install(TARGETS ZeroVirtualConsole RUNTIME DESTINATION .)
