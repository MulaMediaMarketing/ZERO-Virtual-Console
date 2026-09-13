#pragma once
#include <filesystem>
#include <string>

namespace zero {

struct IdentityProfile {
    std::string zeroId;
    std::string displayName;
    bool localOnly{true};
};

class IIdentityProvider {
public:
    virtual ~IIdentityProvider() = default;
    virtual bool Initialize(const std::string& displayName, std::wstring& error) = 0;
    virtual IdentityProfile CurrentProfile() const = 0;
};

class LocalIdentityProvider final : public IIdentityProvider {
public:
    explicit LocalIdentityProvider(std::filesystem::path root);

    bool Initialize(const std::string& displayName, std::wstring& error) override;
    IdentityProfile CurrentProfile() const override { return profile_; }

private:
    bool LoadOrCreateId(std::wstring& error);

    std::filesystem::path root_;
    IdentityProfile profile_;
};

} // namespace zero
