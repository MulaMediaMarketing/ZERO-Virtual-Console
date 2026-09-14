#pragma once

#include "ResumeStore.h"
#include "RuntimeCore.h"
#include <optional>
#include <string>

namespace zero::v5 {

struct ResumeWriteRequest {
    RuntimeSessionGrant grant;
    std::string activityId;
    std::string displayLabel;
    std::string payload;
};

class ResumeCoordinator {
public:
    explicit ResumeCoordinator(zero::ResumeStore store = {}) : store_(std::move(store)) {}

    bool Save(const ResumeWriteRequest& request, std::string& error) const;
    std::optional<zero::ResumeMetadata> LoadForLaunch(const RuntimeSessionGrant& grant,
                                                      std::string& error) const;
    bool ClearForPackage(const RuntimeSessionGrant& grant, std::string& error) const;

private:
    zero::ResumeStore store_;

    static bool HasCapability(const RuntimeSessionGrant& grant, RuntimeCapability capability);
    static bool ValidateText(const std::string& value, size_t maxBytes, bool allowEmpty);
    static bool ValidateGrant(const RuntimeSessionGrant& grant, std::string& error);
};

} // namespace zero::v5
