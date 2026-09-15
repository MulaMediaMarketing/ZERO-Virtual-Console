target_sources(ZeroVirtualConsole PRIVATE
  src/v5/CloudDomain.cpp)

add_executable(ZeroV5CloudAcceptance
  tools/V5CloudAcceptance.cpp
  src/v5/CloudDomain.cpp)
target_include_directories(ZeroV5CloudAcceptance PRIVATE include)
target_compile_definitions(ZeroV5CloudAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
