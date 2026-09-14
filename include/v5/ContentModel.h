#pragma once

#include <cstdint>
#include <string>

namespace zero::v5 {

enum class ContentClass : std::uint8_t {
    ZeroOriginal,
    Premium,
    FreeToPlay,
    Community,
    Demo,
    Dlc,
    Expansion,
    Experience
};

enum class EntitlementType : std::uint8_t {
    Free,
    Purchased,
    Subscription,
    Promotional,
    Trial,
    CreatorGrant,
    Bundle,
    TemporaryAccess
};

enum class RuntimeType : std::uint8_t {
    NativeWin32,
    Unreal,
    Godot,
    Web,
    Zero2D,
    Streaming,
    FutureRuntime
};

struct ContentIdentity {
    std::string contentId;
    std::string packageId;
    std::string publisherId;
    ContentClass contentClass{ContentClass::Premium};
};

struct EntitlementGrant {
    std::string entitlementId;
    std::string accountId;
    std::string contentId;
    EntitlementType type{EntitlementType::Purchased};
    std::string authorityVersion;
    std::string expiresAtUtc;
    bool authoritative{false};
};

struct LaunchDescriptor {
    std::string packageId;
    std::string version;
    RuntimeType runtimeType{RuntimeType::NativeWin32};
    std::string contentRoot;
    std::string executable;
    std::string arguments;
    std::string saveNamespace;
    std::string capabilityScope;
    std::string entitlementToken;
    std::string sessionToken;
};

} // namespace zero::v5
