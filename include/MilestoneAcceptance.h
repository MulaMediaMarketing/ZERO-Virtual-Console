#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace zero {

struct AcceptanceCheck {
    std::string name;
    bool passed{false};
    std::string detail;
};

class MilestoneAcceptance {
public:
    explicit MilestoneAcceptance(std::filesystem::path zeroRoot);
    std::vector<AcceptanceCheck> Run() const;
    bool AllPassed(const std::vector<AcceptanceCheck>& checks) const;
private:
    std::filesystem::path root_;
};

} // namespace zero
