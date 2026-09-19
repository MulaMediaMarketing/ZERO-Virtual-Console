#pragma once

#include "GameRegistry.h"
#include "v5/ProductionRuntime.h"
#include <cstddef>
#include <string>
#include <vector>

namespace zero::features::library {

struct LibraryItemViewModel {
    std::string packageId;
    std::string title;
    std::string version;
    std::uint64_t playtimeSeconds{0};
    std::uint64_t launchCount{0};
    bool selected{false};
    bool resumeAvailable{false};
};

struct LibraryViewModel {
    bool empty{true};
    std::size_t selectedIndex{0};
    std::vector<LibraryItemViewModel> items;
};

class LibraryController final {
public:
    static LibraryViewModel Build(const GameRegistry& registry,
                                  const ProductionRuntime& runtime,
                                  std::size_t selectedIndex);
    static bool MoveSelection(LibraryViewModel& model, int direction) noexcept;
};

class LibraryView final {
public:
    static std::string Summary(const LibraryViewModel& model);
};

} // namespace zero::features::library
