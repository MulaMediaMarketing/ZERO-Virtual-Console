target_sources(ZeroVirtualConsole PRIVATE
  src/v5/DomainRepositories.cpp)

add_executable(ZeroV5DomainRepositoriesAcceptance
  tools/V5DomainRepositoriesAcceptance.cpp
  src/v5/DomainRepositories.cpp
  src/PlatformDatabase.cpp
  src/PlatformDatabaseResume.cpp
  src/PlatformDatabaseReads.cpp)
target_include_directories(ZeroV5DomainRepositoriesAcceptance PRIVATE include)
target_compile_definitions(ZeroV5DomainRepositoriesAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroV5DomainRepositoriesAcceptance PRIVATE shell32 ole32 winsqlite3)
