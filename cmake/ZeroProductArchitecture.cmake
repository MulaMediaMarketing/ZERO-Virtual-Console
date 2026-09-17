# Senior product architecture modules layered above the V5 domain/runtime core.
# Keep composition, shell coordination, and feature presentation sources here so
# the root CMakeLists stays readable as ZERO Player grows.

target_sources(ZeroVirtualConsole PRIVATE
  src/shell/ShellModule.cpp
)
