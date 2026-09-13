#include "CaptureLibrary.h"
#include <algorithm>
#include <cwctype>

namespace zero {
namespace {

bool supportedCapture(const std::filesystem::path& path) {
    std::wstring ext = path.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return ext == L".png" || ext == L".jpg" || ext == L".jpeg" || ext == L".bmp" ||
           ext == L".mp4" || ext == L".webm";
}

} // namespace

CaptureLibrary::CaptureLibrary(std::filesystem::path root) : root_(std::move(root)) {}

void CaptureLibrary::Refresh() {
    items_.clear();
    std::error_code ec;
    std::filesystem::create_directories(root_, ec);
    ec.clear();
    if (!std::filesystem::exists(root_, ec)) return;

    for (std::filesystem::recursive_directory_iterator it(root_, ec), end; !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file(ec) || ec) { ec.clear(); continue; }
        if (!supportedCapture(it->path())) continue;
        CaptureItem item;
        item.path = it->path();
        item.modifiedAt = it->last_write_time(ec);
        if (ec) { ec.clear(); continue; }
        items_.push_back(std::move(item));
    }

    std::sort(items_.begin(), items_.end(), [](const CaptureItem& a, const CaptureItem& b) {
        return a.modifiedAt > b.modifiedAt;
    });
}

} // namespace zero
