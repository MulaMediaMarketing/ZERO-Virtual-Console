#include "v5/ResumeCoordinator.h"
#include "ZeroProtocol.h"
#include <algorithm>

namespace zero::v5 {
namespace {

std::string utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                          value.data(), static_cast<int>(value.size()),
                                          nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return {};
    std::string out(static_cast<size_t>(bytes), '\0');
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                             value.data(), static_cast<int>(value.size()),
                             out.data(), bytes, nullptr, nullptr)) return {};
    return out;
}

} // namespace

bool ResumeCoordinator::HasCapability(const RuntimeSessionGrant& grant,
                                      RuntimeCapability capability) {
    return std::find(grant.grantedCapabilities.begin(), grant.grantedCapabilities.end(), capability) !=
           grant.grantedCapabilities.end();
}

bool ResumeCoordinator::ValidateText(const std::string& value,
                                     size_t maxBytes,
                                     bool allowEmpty) {
    if ((!allowEmpty && value.empty()) || value.size() > maxBytes) return false;
    return zero::protocol::IsValidUtf8(value);
}

bool ResumeCoordinator::ValidateGrant(const RuntimeSessionGrant& grant, std::string& error) {
    if (grant.identity.sessionId.empty() || grant.identity.packageId.empty() ||
        grant.identity.accountId.empty()) {
        error = "resume operation requires an authoritative runtime identity";
        return false;
    }
    return true;
}

bool ResumeCoordinator::Save(const ResumeWriteRequest& request, std::string& error) const {
    if (!ValidateGrant(request.grant, error)) return false;
    if (!HasCapability(request.grant, RuntimeCapability::SaveWrite)) {
        error = "resume write capability was not granted";
        return false;
    }
    if (!ValidateText(request.activityId, 256, false) ||
        !ValidateText(request.displayLabel, 512, false) ||
        !ValidateText(request.payload, 64u * 1024u, true)) {
        error = "resume payload failed V5 bounds or UTF-8 validation";
        return false;
    }

    zero::ResumeMetadata metadata;
    metadata.packageId = request.grant.identity.packageId;
    metadata.activityId = request.activityId;
    metadata.displayLabel = request.displayLabel;
    metadata.payload = request.payload;

    std::wstring storeError;
    if (!store_.Save(metadata, storeError)) {
        error = utf8(storeError);
        if (error.empty()) error = "resume repository rejected the write";
        return false;
    }
    return true;
}

std::optional<zero::ResumeMetadata> ResumeCoordinator::LoadForLaunch(
    const RuntimeSessionGrant& grant,
    std::string& error) const {
    if (!ValidateGrant(grant, error)) return std::nullopt;
    if (!HasCapability(grant, RuntimeCapability::SaveRead)) {
        error = "resume read capability was not granted";
        return std::nullopt;
    }
    auto metadata = store_.Load(grant.identity.packageId);
    if (!metadata) return std::nullopt;
    if (metadata->packageId != grant.identity.packageId ||
        !ValidateText(metadata->activityId, 256, false) ||
        !ValidateText(metadata->displayLabel, 512, false) ||
        !ValidateText(metadata->payload, 64u * 1024u, true)) {
        error = "stored resume metadata failed V5 package binding or bounds validation";
        return std::nullopt;
    }
    return metadata;
}

bool ResumeCoordinator::ClearForPackage(const RuntimeSessionGrant& grant, std::string& error) const {
    if (!ValidateGrant(grant, error)) return false;
    if (!HasCapability(grant, RuntimeCapability::SaveWrite)) {
        error = "resume clear requires save-write capability";
        return false;
    }
    std::wstring storeError;
    if (!store_.Clear(grant.identity.packageId, storeError)) {
        error = utf8(storeError);
        if (error.empty()) error = "resume repository rejected the clear operation";
        return false;
    }
    return true;
}

} // namespace zero::v5
