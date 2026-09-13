cmake_minimum_required(VERSION 3.24)
project(ZeroVirtualConsole LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
  message(FATAL_ERROR "ZERO Virtual Console currently targets Windows 11 x64 only.")
endif()

add_library(ZeroSdk STATIC
  sdk/ZeroSdk.cpp
  sdk/ZeroSdkCrash.cpp
)
target_include_directories(ZeroSdk PUBLIC sdk include)
target_compile_definitions(ZeroSdk PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroSdk PUBLIC dbghelp)

add_executable(ZeroVirtualConsole WIN32
  src/main.cpp
  src/App.cpp
  src/CaptureLibrary.cpp
  src/LocalIdentityProvider.cpp
  src/GameRegistry.cpp
  src/GameImportService.cpp
  src/ManifestValidator.cpp
  src/PackageSecurity.cpp
  src/RuntimeSession.cpp
  src/RuntimeSessionLifecycle.cpp
  src/RuntimeSessionDiagnostics.cpp
  src/RuntimeIpcServer.cpp
  src/RuntimeV3.cpp
  src/RuntimeV4.cpp
  src/ResumeStore.cpp
  src/PlatformStateStore.cpp
  src/PlatformDatabase.cpp
  src/PlatformDatabaseResume.cpp
  src/AchievementStore.cpp
  src/CrashReportStore.cpp
  src/MiniDumpWriter.cpp
  src/FirstBootService.cpp
  src/FirstBootWizard.cpp
  src/MilestoneAcceptance.cpp
  src/Settings.cpp
  src/Input.cpp
)

target_include_directories(ZeroVirtualConsole PRIVATE include)
target_compile_definitions(ZeroVirtualConsole PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroVirtualConsole PRIVATE d2d1 dwrite windowscodecs xinput9_1_0 shlwapi shell32 ole32 bcrypt advapi32 dbghelp winsqlite3)

add_executable(ZeroTestGame sample_game/TestGame.cpp)
target_include_directories(ZeroTestGame PRIVATE sdk include)
target_compile_definitions(ZeroTestGame PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroTestGame PRIVATE ZeroSdk)

add_executable(ZeroReferenceGame sample_game/reference/ReferenceGame.cpp)
target_include_directories(ZeroReferenceGame PRIVATE sdk include)
target_compile_definitions(ZeroReferenceGame PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroReferenceGame PRIVATE ZeroSdk)

add_executable(ZeroAcceptance
  tools/AcceptanceMain.cpp
  src/MilestoneAcceptance.cpp
  src/PlatformDatabase.cpp
  src/PlatformDatabaseResume.cpp
)
target_include_directories(ZeroAcceptance PRIVATE include)
target_compile_definitions(ZeroAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroAcceptance PRIVATE shell32 ole32 winsqlite3)

install(TARGETS ZeroVirtualConsole ZeroAcceptance RUNTIME DESTINATION .)
