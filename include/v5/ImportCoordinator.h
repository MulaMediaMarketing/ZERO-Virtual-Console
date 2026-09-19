#pragma once

#include "GameImportService.h"
#include <filesystem>

namespace zero::v5 {

class ImportCoordinator final {
public:
    explicit ImportCoordinator(std::filesystem::path libraryRoot);

    zero::ImportResult ImportFolder(const std::filesystem::path& sourceRoot) const;
    zero::ImportResult RepairFolder(const std::filesystem::path& sourceRoot,
                                    const std::string& expectedPackageId) const;

private:
    zero::GameImportService importer_;
};

} // namespace zero::v5
