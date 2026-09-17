#pragma once

#include "CaptureLibrary.h"
#include "CapturesExperience.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace zero::features::capture {

struct CaptureItemViewModel {
    std::filesystem::path path;
    CaptureKind kind{CaptureKind::Screenshot};
    std::uint64_t sizeBytes{0};
    bool selected{false};
    bool canViewInline{false};
    bool canOpenExternally{false};
};

struct CaptureViewModel {
    CaptureExperienceMode mode{CaptureExperienceMode::Empty};
    std::size_t selectedIndex{0};
    std::vector<CaptureItemViewModel> items;
};

class CaptureController final {
public:
    static CaptureViewModel Build(const CaptureLibrary& library,
                                  const CaptureExperienceState& state);
    static bool MoveSelection(CaptureViewModel& model, int direction) noexcept;
};

class CaptureView final {
public:
    static std::string KindLabel(CaptureKind kind);
};

} // namespace zero::features::capture
