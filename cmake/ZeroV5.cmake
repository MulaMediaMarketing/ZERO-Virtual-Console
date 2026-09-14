# ZERO Core Architecture V5 production migration targets.
# Kept separate from the V4.1 target list so migration ownership remains visible.

target_sources(ZeroVirtualConsole PRIVATE
  src/v5/ProductionPackagePlatform.cpp)

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
