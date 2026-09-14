#pragma once

#include "ContentModel.h"
#include "PackageTrust.h"
#include <filesystem>
#include <optional>
#include <string>

namespace zero::v5 {

enum class LaunchRequestMode : unsigned char {
    LocalInstalled,
    Managed
};

struct ProductionLaunchRequest {
    LaunchRequestMode mode{LaunchRequestMode::Managed};
    std::string contentId;
    std::string accountId;
};

struct ProductionLaunchResult {
    bool authorized{false};
    LaunchDescriptor descriptor;
    std::wstring error;
};

class IManagedLaunchAuthorityClient {
public:
    virtual ~IManagedLaunchAuthorityClient() = default;
    virtual std::optional<LaunchDescriptor> Authorize(const std::string& accountId,
                                                       const std::string& contentId,
                                                       std::wstring& error) const = 0;
};

class DisconnectedManagedLaunchAuthorityClient final : public IManagedLaunchAuthorityClient {
public:
    std::optional<LaunchDescriptor> Authorize(const std::string& accountId,
                                               const std::string& contentId,
                                               std::wstring& error) const override;
};

class ProductionPlatformKernel final {
public:
    ProductionPlatformKernel(std::filesystem::path libraryRoot,
                             const IPublisherTrustProvider& trustProvider,
                             const IManagedLaunchAuthorityClient& managedLaunchAuthority);

    ProductionLaunchResult RequestLaunch(const ProductionLaunchRequest& request) const;

private:
    ProductionLaunchResult RequestLocalInstalled(const std::string& contentId) const;

    std::filesystem::path libraryRoot_;
    const IPublisherTrustProvider& trustProvider_;
    const IManagedLaunchAuthorityClient& managedLaunchAuthority_;
};

} // namespace zero::v5
