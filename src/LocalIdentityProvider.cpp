#include "IdentityProvider.h"
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace zero {
namespace {

std::string hexId(const std::array<unsigned char, 16>& bytes) {
    std::ostringstream out;
    out << "zero-local-" << std::hex << std::setfill('0');
    for (unsigned char byte : bytes) out << std::setw(2) << static_cast<unsigned int>(byte);
    return out.str();
}

bool validLocalId(const std::string& value) {
    constexpr size_t kPrefix = 11; // "zero-local-"
    if (value.size() != kPrefix + 32 || value.rfind("zero-local-", 0) != 0) return false;
    for (size_t i = kPrefix; i < value.size(); ++i) {
        const char c = value[i];
        const bool hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        if (!hex) return false;
    }
    return true;
}

} // namespace

LocalIdentityProvider::LocalIdentityProvider(std::filesystem::path root)
    : root_(std::move(root)) {}

bool LocalIdentityProvider::Initialize(const std::string& displayName, std::wstring& error) {
    profile_.displayName = displayName.empty() ? "Player" : displayName;
    profile_.localOnly = true;
    return LoadOrCreateId(error);
}

bool LocalIdentityProvider::LoadOrCreateId(std::wstring& error) {
    std::error_code ec;
    const auto identityDir = root_ / "Identity";
    std::filesystem::create_directories(identityDir, ec);
    if (ec) {
        error = L"ZERO could not create the local identity directory.";
        return false;
    }

    const auto path = identityDir / "local.id";
    if (std::filesystem::exists(path, ec) && !ec) {
        std::ifstream in(path, std::ios::binary);
        std::string value;
        std::getline(in, value);
        if (validLocalId(value)) {
            profile_.zeroId = std::move(value);
            return true;
        }
        error = L"ZERO found an invalid local identity record.";
        return false;
    }

    std::array<unsigned char, 16> random{};
    if (BCryptGenRandom(nullptr, random.data(), static_cast<ULONG>(random.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        error = L"ZERO could not securely create a local identity.";
        return false;
    }

    const std::string value = hexId(random);
    const auto temp = identityDir / "local.id.tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) {
            error = L"ZERO could not persist the local identity.";
            return false;
        }
        out << value << "\n";
        out.flush();
        if (!out) {
            error = L"ZERO could not persist the local identity.";
            return false;
        }
    }

    std::filesystem::rename(temp, path, ec);
    if (ec) {
        std::filesystem::remove(temp);
        error = L"ZERO could not finalize the local identity record.";
        return false;
    }

    profile_.zeroId = value;
    return true;
}

} // namespace zero
