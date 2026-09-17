#pragma once

#include "CaptureLibrary.h"
#include "FriendsProvider.h"
#include "GameRegistry.h"
#include "IdentityProvider.h"
#include "Input.h"
#include "Settings.h"
#include "StoreProvider.h"
#include "shell/ShellModule.h"
#include "v5/ImportCoordinator.h"
#include "v5/LibraryDownloadAuthority.h"
#include "v5/ProductionRuntime.h"
#include "v5/ProductionShellIntegration.h"
#include <filesystem>

namespace zero {

// ApplicationServices is the production composition root for ZERO Player.
//
// The shell must never construct feature authorities directly. All long-lived
// production services are created here, then injected into App. This keeps
// ownership obvious, prevents duplicate sources of truth, and provides one
// replacement point when disconnected transports are replaced by real ZERO
// backend adapters.
class ApplicationServices final {
public:
    explicit ApplicationServices(const std::filesystem::path& dataRoot);

    GameRegistry& Registry() noexcept { return registry_; }
    v5::ImportCoordinator& Importer() noexcept { return importer_; }
    v5::DownloadAuthority& Downloads() noexcept { return downloads_; }
    CaptureLibrary& Captures() noexcept { return captures_; }
    IIdentityProvider& Identity() noexcept { return identity_; }
    IFriendsProvider& Friends() noexcept { return friends_; }
    IStoreProvider& Store() noexcept { return store_; }
    ProductionRuntime& Runtime() noexcept { return runtime_; }
    SettingsStore& Settings() noexcept { return settingsStore_; }
    Input& PlayerInput() noexcept { return input_; }
    v5::ProductionShellIntegration& Shell() noexcept { return shell_; }
    shell::ShellModule& ShellCoordinator() noexcept { return shellModule_; }

private:
    GameRegistry registry_;
    v5::ImportCoordinator importer_;
    v5::DownloadAuthority downloads_;
    CaptureLibrary captures_;

    // These are deliberately concrete disconnected/local adapters today.
    // Replacing them with production network adapters happens here rather than
    // inside views, renderers, or feature code.
    LocalIdentityProvider identity_;
    DisconnectedFriendsProvider friends_;
    DisconnectedStoreProvider store_;

    ProductionRuntime runtime_;
    SettingsStore settingsStore_;
    Input input_;
    v5::ProductionShellIntegration shell_;
    shell::ShellModule shellModule_;
};

} // namespace zero
