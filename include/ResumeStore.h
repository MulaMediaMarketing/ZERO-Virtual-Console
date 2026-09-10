#pragma once
#include <filesystem>
#include <optional>
#include <string>

namespace zero {

struct ResumeMetadata {
    std::string packageId;
    std::string activityId;
    std::string displayLabel;
    std::string payload;
    std::string updatedAtUtc;
};

class ResumeStore {
public:
    bool Save(const ResumeMetadata& metadata, std::wstring& error) const;
    std::optional<ResumeMetadata> Load(const std::string& packageId) const;
    bool Clear(const std::string& packageId, std::wstring& error) const;

private:
    std::filesystem::path PathFor(const std::string& packageId) const;
};

} // namespace zero
