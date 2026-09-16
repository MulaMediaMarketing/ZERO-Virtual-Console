target_sources(ZeroVirtualConsole PRIVATE
  src/v5/NotificationDomain.cpp)

add_executable(ZeroV5NotificationAcceptance
  tools/V5NotificationAcceptance.cpp
  src/v5/NotificationDomain.cpp)
target_include_directories(ZeroV5NotificationAcceptance PRIVATE include)
target_compile_definitions(ZeroV5NotificationAcceptance PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
