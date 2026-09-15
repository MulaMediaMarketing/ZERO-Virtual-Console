target_sources(ZeroVirtualConsole PRIVATE
  src/v5/CommerceDomain.cpp)

add_executable(ZeroV5CommerceAcceptance
  tools/V5CommerceAcceptance.cpp
  src/v5/CommerceDomain.cpp)
target_include_directories(ZeroV5CommerceAcceptance PRIVATE include)
target_compile_definitions(ZeroV5CommerceAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
