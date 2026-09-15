target_sources(ZeroVirtualConsole PRIVATE
  src/v5/ZeroIdAuthority.cpp)

add_executable(ZeroV5ZeroIdAcceptance
  tools/V5ZeroIdAcceptance.cpp
  src/v5/ZeroIdAuthority.cpp)
target_include_directories(ZeroV5ZeroIdAcceptance PRIVATE include)
target_compile_definitions(ZeroV5ZeroIdAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
