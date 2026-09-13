#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace zero {

enum class CaptureKind {
    Screenshot,
    Video
};

struct CaptureItem {
    std::filesystem::path path;
    std::filesystem::file_time_type modifiedAt{};
    CaptureKind kind{CaptureKind::Screenshot};
    uint64_t sizeBytes{0};
};

class CaptureLibrary {
public:
    explicit CaptureLibrary(std::filesystem::path root);

    void Refresh();
    const std::vector<CaptureItem>& Items() const noexcept { return items_; }
    const std::filesystem::path& Root() const noexcept { return root_; }

    bool Delete(const CaptureItem& item, std::wstring& error);
    bool Open(const CaptureItem& item, std::wstring& error) const;
    bool Reveal(const CaptureItem& item, std::wstring& error) const;

    static bool IsScreenshot(const std::filesystem::path& path);
    static bool IsVideo(const std::filesystem::path& path);

private:
    bool IsInsideRoot(const std::filesystem::path& path) const;

    std::filesystem::path root_;
    std::vector<CaptureItem> items_;
};

} // namespace zero
