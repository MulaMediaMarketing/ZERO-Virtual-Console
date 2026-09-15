target_sources(ZeroVirtualConsole PRIVATE
  src/v5/SocialDomain.cpp)

add_executable(ZeroV5SocialAcceptance
  tools/V5SocialAcceptance.cpp
  src/v5/SocialDomain.cpp)
target_include_directories(ZeroV5SocialAcceptance PRIVATE include)
target_compile_definitions(ZeroV5SocialAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
