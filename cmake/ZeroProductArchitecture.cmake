# Senior product architecture modules layered above the V5 domain/runtime core.
# Keep composition, shell coordination, and feature presentation sources here so
# the root CMakeLists stays readable as ZERO Player grows.

target_sources(ZeroVirtualConsole PRIVATE
  src/shell/ShellModule.cpp
  src/features/home/HomeFeature.cpp
  src/features/library/LibraryFeature.cpp
  src/features/store/StoreFeature.cpp
  src/features/downloads/DownloadsFeature.cpp
  src/features/friends/FriendsFeature.cpp
  src/features/achievements/AchievementsFeature.cpp
  src/features/capture/CaptureFeature.cpp
  src/features/profile/ProfileFeature.cpp
  src/features/devices/DevicesFeature.cpp
  src/features/notifications/NotificationsFeature.cpp
  src/features/settings/SettingsFeature.cpp
)

add_executable(ZeroFeatureArchitectureAcceptance
  tools/FeatureArchitectureAcceptance.cpp
)
target_include_directories(ZeroFeatureArchitectureAcceptance PRIVATE include)
target_compile_definitions(ZeroFeatureArchitectureAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
