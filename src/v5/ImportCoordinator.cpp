#include "v5/ImportCoordinator.h"

namespace zero::v5 {

ImportCoordinator::ImportCoordinator(std::filesystem::path libraryRoot)
    : importer_(std::move(libraryRoot)) {}

zero::ImportResult ImportCoordinator::ImportFolder(const std::filesystem::path& sourceRoot) const {
    return importer_.ImportFolder(sourceRoot);
}

zero::ImportResult ImportCoordinator::RepairFolder(const std::filesystem::path& sourceRoot,
                                                    const std::string& expectedPackageId) const {
    return importer_.RepairFolder(sourceRoot, expectedPackageId);
}

} // namespace zero::v5
