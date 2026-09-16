target_sources(ZeroVirtualConsole PRIVATE
  src/v5/PlayerUpdateDomain.cpp)

add_executable(ZeroV5PlayerUpdateAcceptance
  tools/V5PlayerUpdateAcceptance.cpp
  src/v5/PlayerUpdateDomain.cpp)
target_include_directories(ZeroV5PlayerUpdateAcceptance PRIVATE include)
target_compile_definitions(ZeroV5PlayerUpdateAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
