add_executable(ZeroV5FinalCutoverAcceptance
  tools/V5FinalCutoverAcceptance.cpp
  src/v5/ProductionRuntime.cpp
  src/v5/ProductionRuntimeAchievementProgress.cpp
  src/v5/AchievementProfileDomain.cpp
  src/v5/RuntimeAuthority.cpp
  src/v5/NativeRuntimeProcessHost.cpp
  src/v5/CrashSupervisor.cpp
  src/v5/ResumeCoordinator.cpp
  src/v5/DomainRepositories.cpp
  src/RuntimeIpcServer.cpp
  src/RuntimeSession.cpp
  src/RuntimeSessionLifecycle.cpp
  src/RuntimeSessionDiagnostics.cpp
  src/MiniDumpWriter.cpp
  src/PackageSecurity.cpp
  src/PackageIntegrityVerifier.cpp
  src/PackageTrust.cpp
  src/CngPublisherTrustProvider.cpp
  src/PlatformDatabase.cpp
  src/PlatformDatabaseResume.cpp
  src/PlatformDatabaseReads.cpp
  src/PlatformStateStore.cpp
  src/AchievementStore.cpp
  src/ResumeStore.cpp
  src/CrashReportStore.cpp)
target_include_directories(ZeroV5FinalCutoverAcceptance PRIVATE include)
target_compile_definitions(ZeroV5FinalCutoverAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroV5FinalCutoverAcceptance PRIVATE
  shell32 ole32 winsqlite3 bcrypt advapi32 dbghelp)
