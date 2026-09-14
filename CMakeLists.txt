cmake_minimum_required(VERSION 3.24)
project(ZeroVirtualConsole LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
  message(FATAL_ERROR "ZERO Virtual Console currently targets Windows 11 x64 only.")
endif()

if(MSVC)
  add_compile_options(/W4 /WX /permissive- /utf-8)
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
  src/AppPages.cpp
  src/AppAchievements.cpp
  src/AppLaunchRecovery.cpp
  src/AppSettings.cpp
  src/AppUx.cpp
  src/CaptureLibrary.cpp
  src/LocalIdentityProvider.cpp
  src/GameRegistry.cpp
  src/GameImportService.cpp
  src/PackageManifestParser.cpp
  src/ManifestValidator.cpp
  src/PackageSecurity.cpp
  src/PackageIntegrityVerifier.cpp
  src/PackageTrust.cpp
  src/PackageTrustPresentation.cpp
  src/CngPublisherTrustProvider.cpp
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
  src/PlatformDatabaseReads.cpp
  src/AchievementStore.cpp
  src/CrashReportStore.cpp
  src/MiniDumpWriter.cpp
  src/FirstBootService.cpp
  src/FirstBootWizard.cpp
  src/MilestoneAcceptance.cpp
  src/Settings.cpp
  src/InputCore.cpp
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
  src/PlatformDatabaseReads.cpp
)
target_include_directories(ZeroAcceptance PRIVATE include)
target_compile_definitions(ZeroAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroAcceptance PRIVATE shell32 ole32 winsqlite3)

add_executable(ZeroIpcPersistenceAcceptance
  tools/IpcPersistenceAcceptance.cpp
  src/RuntimeIpcServer.cpp
)
target_include_directories(ZeroIpcPersistenceAcceptance PRIVATE sdk include)
target_compile_definitions(ZeroIpcPersistenceAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroIpcPersistenceAcceptance PRIVATE ZeroSdk advapi32)

add_executable(ZeroPackageIntegrityAcceptance
  tools/PackageIntegrityAcceptance.cpp
  src/PackageSecurity.cpp
  src/PackageIntegrityVerifier.cpp
)
target_include_directories(ZeroPackageIntegrityAcceptance PRIVATE include)
target_compile_definitions(ZeroPackageIntegrityAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroPackageIntegrityAcceptance PRIVATE bcrypt)

add_executable(ZeroPackageTrustAcceptance
  tools/PackageTrustAcceptance.cpp
  src/PackageTrust.cpp
  src/PackageTrustPresentation.cpp
  src/CngPublisherTrustProvider.cpp
)
target_include_directories(ZeroPackageTrustAcceptance PRIVATE include)
target_compile_definitions(ZeroPackageTrustAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroPackageTrustAcceptance PRIVATE bcrypt shell32 ole32)

add_executable(ZeroPackageRepairAcceptance
  tools/PackageRepairAcceptance.cpp
  src/GameImportService.cpp
  src/GameRegistry.cpp
  src/PackageManifestParser.cpp
  src/ManifestValidator.cpp
  src/PackageSecurity.cpp
  src/PackageIntegrityVerifier.cpp
)
target_include_directories(ZeroPackageRepairAcceptance PRIVATE include)
target_compile_definitions(ZeroPackageRepairAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroPackageRepairAcceptance PRIVATE bcrypt)

add_executable(ZeroStrictJsonAcceptance tools/StrictJsonAcceptance.cpp)
target_include_directories(ZeroStrictJsonAcceptance PRIVATE include)
target_compile_definitions(ZeroStrictJsonAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

add_executable(ZeroInputAcceptance
  tools/InputAcceptance.cpp
  src/InputCore.cpp
)
target_include_directories(ZeroInputAcceptance PRIVATE include)
target_compile_definitions(ZeroInputAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

add_executable(ZeroShellUxAcceptance tools/ShellUxAcceptance.cpp)
target_include_directories(ZeroShellUxAcceptance PRIVATE include)
target_compile_definitions(ZeroShellUxAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

add_executable(ZeroProductionUxAcceptance tools/ProductionUxAcceptance.cpp)
target_include_directories(ZeroProductionUxAcceptance PRIVATE include)
target_compile_definitions(ZeroProductionUxAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

add_executable(ZeroFriendsExperienceAcceptance tools/FriendsExperienceAcceptance.cpp)
target_include_directories(ZeroFriendsExperienceAcceptance PRIVATE include)
target_compile_definitions(ZeroFriendsExperienceAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

add_executable(ZeroCapturesExperienceAcceptance tools/CapturesExperienceAcceptance.cpp)
target_include_directories(ZeroCapturesExperienceAcceptance PRIVATE include)
target_compile_definitions(ZeroCapturesExperienceAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

add_executable(ZeroCoreShellExperienceAcceptance tools/CoreShellExperienceAcceptance.cpp)
target_include_directories(ZeroCoreShellExperienceAcceptance PRIVATE include)
target_compile_definitions(ZeroCoreShellExperienceAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

add_executable(ZeroStoreSettingsFirstBootAcceptance tools/StoreSettingsFirstBootAcceptance.cpp)
target_include_directories(ZeroStoreSettingsFirstBootAcceptance PRIVATE include)
target_compile_definitions(ZeroStoreSettingsFirstBootAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

add_executable(ZeroFirstBootPersistenceAcceptance
  tools/FirstBootPersistenceAcceptance.cpp
  src/FirstBootService.cpp
)
target_include_directories(ZeroFirstBootPersistenceAcceptance PRIVATE include)
target_compile_definitions(ZeroFirstBootPersistenceAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

add_executable(ZeroQualificationProbe tools/QualificationProbe.cpp)
target_compile_definitions(ZeroQualificationProbe PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroQualificationProbe PRIVATE xinput9_1_0)

install(TARGETS ZeroVirtualConsole ZeroAcceptance RUNTIME DESTINATION .)
