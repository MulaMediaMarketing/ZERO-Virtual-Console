#pragma once
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
