#include "ApplicationServices.h"
#include "PlatformPaths.h"

namespace zero {

ApplicationServices::ApplicationServices(const std::filesystem::path& dataRoot)
    : registry_(PlatformPaths::LibraryRoot()),
      importer_(PlatformPaths::LibraryRoot()),
      captures_(PlatformPaths::CapturesRoot()),
      identity_(dataRoot),
      settingsStore_(dataRoot),
      shellModule_(shell_),
      serviceState_(registry_, captures_, friends_, store_, runtime_) {}

} // namespace zero
