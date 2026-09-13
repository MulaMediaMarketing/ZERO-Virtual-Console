#pragma once
#include <filesystem>
#include <vector>

namespace zero {

struct CaptureItem {
    std::filesystem::path path;
    std::filesystem::file_time_type modifiedAt{};
};

class CaptureLibrary {
public:
    explicit CaptureLibrary(std::filesystem::path root);
    void Refresh();
    const std::vector<CaptureItem>& Items() const noexcept { return items_; }
    const std::filesystem::path& Root() const noexcept { return root_; }

private:
    std::filesystem::path root_;
    std::vector<CaptureItem> items_;
};

} // namespace zero
