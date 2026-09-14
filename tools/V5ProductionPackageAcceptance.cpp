#include "PackageTrust.h"
#include "v5/ProductionPackagePlatform.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

struct TempRoot {
    std::filesystem::path path;
    ~TempRoot() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

bool writeText(const std::filesystem::path& path, const std::string& text) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) return false;
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << text;
    return stream.good();
}

bool writePackage(const std::filesystem::path& root, const std::string& version) {
    const std::string manifest =
        "{\n"
        "  \"schema\": 1,\n"
        "  \"minimum_runtime_major\": 4,\n"
        "  \"package_id\": \"zero.acceptance.v5package\",\n"
        "  \"title\": \"ZERO V5 Package Acceptance\",\n"
        "  \"version\": \"" + version + "\",\n"
        "  \"executable\": \"game.exe\",\n"
        "  \"hero_image\": \"Assets/hero.png\",\n"
        "  \"icon_image\": \"Assets/icon.png\",\n"
        "  \"logo_image\": \"Assets/logo.png\",\n"
        "  \"zero_resume\": true,\n"
        "  \"zero_achievements\": true,\n"
        "  \"zero_overlay\": true,\n"
        "  \"zero_input\": true\n"
        "}\n";
    return writeText(root / L"zero.manifest.json", manifest) &&
           writeText(root / L"game.exe", "ZERO-V5-ACCEPTANCE") &&
           writeText(root / L"Assets" / L"hero.png", "hero") &&
           writeText(root / L"Assets" / L"icon.png", "icon") &&
           writeText(root / L"Assets" / L"logo.png", "logo");
}

} // namespace

int main() {
    using namespace zero;
    using namespace zero::v5;

    TempRoot temp{std::filesystem::temp_directory_path() / L"zero-v5-package-acceptance"};
    std::error_code ec;
    std::filesystem::remove_all(temp.path, ec);
    const auto source = temp.path / L"source";
    const auto library = temp.path / L"library";

    if (!writePackage(source, "1.0.0")) return 10;

    DisconnectedPublisherTrustProvider trust;
    ProductionPackagePlatform platform(trust);

    PackageOperationRequest strictRequest;
    strictRequest.sourceRoot = source;
    strictRequest.libraryRoot = library;
    strictRequest.trustPolicy = PackageTrustPolicy::RequireTrustedPublisher;
    strictRequest.contentClass = ContentClass::Premium;
    const auto strictResult = platform.Execute(strictRequest);
    if (strictResult.success || strictResult.stage != PackageOperationStage::VerifyTrust) return 11;

    PackageOperationRequest localRequest = strictRequest;
    localRequest.trustPolicy = PackageTrustPolicy::AllowLocalUnsigned;
    localRequest.contentClass = ContentClass::Community;
    const auto installed = platform.Execute(localRequest);
    if (!installed.success || installed.stage != PackageOperationStage::Ready) return 20;
    if (installed.identity.packageId != "zero.acceptance.v5package") return 21;
    if (installed.version != "1.0.0") return 22;
    if (!std::filesystem::exists(installed.installedRoot / L"zero.integrity.sha256")) return 23;

    if (!writePackage(source, "1.0.1")) return 30;
    PackageOperationRequest repair = localRequest;
    repair.operation = PackageOperation::Repair;
    repair.expectedPackageId = "zero.acceptance.v5package";
    const auto repaired = platform.Execute(repair);
    if (!repaired.success || repaired.version != "1.0.1") return 31;

    repair.expectedPackageId = "zero.acceptance.other";
    const auto wrongRepair = platform.Execute(repair);
    if (wrongRepair.success) return 32;

    std::cout << "ZERO V5 production package migration acceptance: PASS\n";
    return 0;
}
