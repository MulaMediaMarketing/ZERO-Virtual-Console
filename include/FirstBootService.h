#pragma once
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

namespace zero {

struct FirstBootState {
    bool completed{false};
    std::string profileName{"Player"};
    bool controllerConfirmed{false};
    bool controllerDetected{false};
    bool displayConfirmed{false};
    bool audioConfirmed{false};
    unsigned volume{80};
    unsigned displayWidth{0};
    unsigned displayHeight{0};
};

inline bool FirstBootProfileValid(const std::string& value) noexcept {
    if (value.empty() || value.size() > 64) return false;
    return std::any_of(value.begin(), value.end(), [](unsigned char c) { return !std::isspace(c); });
}

inline bool FirstBootRequirementsSatisfied(const FirstBootState& state) noexcept {
    return FirstBootProfileValid(state.profileName) &&
           state.controllerConfirmed &&
           state.displayConfirmed &&
           state.audioConfirmed &&
           state.volume <= 100;
}

inline FirstBootState NormalizeFirstBootState(FirstBootState state) noexcept {
    if (state.volume > 100) state.volume = 100;
    if (!FirstBootRequirementsSatisfied(state)) state.completed = false;
    return state;
}

class FirstBootService {
public:
    explicit FirstBootService(std::filesystem::path zeroRoot);
    FirstBootState Load() const;
    bool Save(const FirstBootState& state, std::wstring& error) const;
    bool IsRequired() const;
private:
    std::filesystem::path root_;
    std::filesystem::path StatePath() const;
};

} // namespace zero
