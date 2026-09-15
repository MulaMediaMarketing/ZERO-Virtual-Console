# ZERO Core Architecture V5 production migration targets.
# Kept separate from the V4.1 target list so migration ownership remains visible.

target_sources(ZeroVirtualConsole PRIVATE
  src/v5/ProductionPackagePlatform.cpp
  src/v5/RuntimeAuthority.cpp
  src/v5/NativeRuntimeProcessHost.cpp)

add_executable(ZeroV5ProductionPackageAcceptance
  tools/V5ProductionPackageAcceptance.cpp
  src/v5/ProductionPackagePlatform.cpp
  src/GameImportService.cpp
  src/PackageManifestParser.cpp
  src/ManifestValidator.cpp
  src/PackageSecurity.cpp
  src/PackageIntegrityVerifier.cpp
  src/PackageTrust.cpp)
target_include_directories(ZeroV5ProductionPackageAcceptance PRIVATE include)
target_compile_definitions(ZeroV5ProductionPackageAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroV5ProductionPackageAcceptance PRIVATE bcrypt)

add_executable(ZeroV5RuntimeAuthorityAcceptance
  tools/V5RuntimeAuthorityAcceptance.cpp
  src/v5/RuntimeAuthority.cpp)
target_include_directories(ZeroV5RuntimeAuthorityAcceptance PRIVATE include)
target_compile_definitions(ZeroV5RuntimeAuthorityAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

add_executable(ZeroV5RuntimeChild tools/V5RuntimeChild.cpp)
target_compile_definitions(ZeroV5RuntimeChild PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

add_executable(ZeroV5NativeRuntimeAcceptance
  tools/V5NativeRuntimeAcceptance.cpp
  src/v5/NativeRuntimeProcessHost.cpp
  src/RuntimeIpcServer.cpp
  src/RuntimeSession.cpp
  src/RuntimeSessionLifecycle.cpp
  src/RuntimeSessionDiagnostics.cpp
  src/MiniDumpWriter.cpp)
target_include_directories(ZeroV5NativeRuntimeAcceptance PRIVATE include)
target_compile_definitions(ZeroV5NativeRuntimeAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroV5NativeRuntimeAcceptance PRIVATE advapi32 ole32 dbghelp)
