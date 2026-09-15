add_executable(ZeroV5SettingsPrivacyAcceptance
  tools/V5SettingsPrivacyAcceptance.cpp
  src/Settings.cpp
  src/PlatformDatabase.cpp
  src/PlatformDatabaseResume.cpp
  src/PlatformDatabaseReads.cpp)
target_include_directories(ZeroV5SettingsPrivacyAcceptance PRIVATE include)
target_compile_definitions(ZeroV5SettingsPrivacyAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ZeroV5SettingsPrivacyAcceptance PRIVATE shell32 ole32 winsqlite3)
