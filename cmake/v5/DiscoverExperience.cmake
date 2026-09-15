target_sources(ZeroVirtualConsole PRIVATE
  src/v5/DiscoverExperience.cpp)

add_executable(ZeroV5DiscoverExperienceAcceptance
  tools/V5DiscoverExperienceAcceptance.cpp
  src/v5/DiscoverExperience.cpp
  src/v5/DiscoverDomain.cpp)
target_include_directories(ZeroV5DiscoverExperienceAcceptance PRIVATE include)
target_compile_definitions(ZeroV5DiscoverExperienceAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
