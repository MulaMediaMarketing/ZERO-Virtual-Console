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

// LocalPackage is valid only for explicitly local/imported content. Managed ZERO
// catalog content must be authorized by ZeroService. This prevents cached/local
// state from silently becoming authority for commerce or online access.
enum class AuthoritySource : std::uint8_t {
    None,
    LocalPackage,
    ZeroService
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
    AuthoritySource authority{AuthoritySource::None};
    std::string authorityVersion;
    std::string expiresAtUtc;
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
