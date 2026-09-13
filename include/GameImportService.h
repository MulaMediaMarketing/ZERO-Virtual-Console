#pragma once
#include <filesystem>
#include <string>

namespace zero {

struct ImportResult {
    bool success{false};
    std::filesystem::path installedRoot;
    std::wstring error;
};

class GameImportService {
public:
    explicit GameImportService(std::filesystem::path libraryRoot);
    ImportResult ImportFolder(const std::filesystem::path& sourceRoot) const;
private:
    std::filesystem::path libraryRoot_;
};

} // namespace zero
