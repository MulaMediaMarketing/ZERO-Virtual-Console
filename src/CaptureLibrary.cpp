#include "CaptureLibrary.h"
#include <algorithm>
#include <cwctype>
#include <shellapi.h>

namespace zero {
namespace {

std::wstring lowerExtension(const std::filesystem::path& path) {
    std::wstring ext = path.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::towlower(c));
    });
    return ext;
}

bool launchShell(const wchar_t* verb,
                 const std::filesystem::path& target,
                 const wchar_t* parameters,
                 std::wstring& error) {
    const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(
        nullptr,
        verb,
        target.c_str(),
        parameters,
        nullptr,
        SW_SHOWNORMAL));
    if (result <= 32) {
        error = L"Windows could not open the selected capture.";
        return false;
    }
    return true;
}

} // namespace

CaptureLibrary::CaptureLibrary(std::filesystem::path root) : root_(std::move(root)) {}

bool CaptureLibrary::IsScreenshot(const std::filesystem::path& path) {
    const auto ext = lowerExtension(path);
    return ext == L".png" || ext == L".jpg" || ext == L".jpeg" || ext == L".bmp";
}

bool CaptureLibrary::IsVideo(const std::filesystem::path& path) {
    const auto ext = lowerExtension(path);
    return ext == L".mp4" || ext == L".webm";
}

bool CaptureLibrary::IsInsideRoot(const std::filesystem::path& path) const {
    std::error_code ec;
    const auto canonicalRoot = std::filesystem::weakly_canonical(root_, ec);
    if (ec) return false;
    ec.clear();
    const auto canonicalPath = std::filesystem::weakly_canonical(path, ec);
    if (ec) return false;

    auto r = canonicalRoot.begin();
    auto p = canonicalPath.begin();
    for (; r != canonicalRoot.end(); ++r, ++p) {
        if (p == canonicalPath.end() || *r != *p) return false;
    }
    return true;
}

void CaptureLibrary::Refresh() {
    items_.clear();
    std::error_code ec;
    std::filesystem::create_directories(root_, ec);
    ec.clear();
    if (!std::filesystem::exists(root_, ec)) return;

    for (std::filesystem::recursive_directory_iterator it(root_, ec), end; !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file(ec) || ec) { ec.clear(); continue; }
        const auto& path = it->path();
        const bool screenshot = IsScreenshot(path);
        const bool video = IsVideo(path);
        if (!screenshot && !video) continue;

        CaptureItem item;
        item.path = path;
        item.kind = screenshot ? CaptureKind::Screenshot : CaptureKind::Video;
        item.modifiedAt = it->last_write_time(ec);
        if (ec) { ec.clear(); continue; }
        item.sizeBytes = static_cast<uint64_t>(it->file_size(ec));
        if (ec) { ec.clear(); item.sizeBytes = 0; }
        items_.push_back(std::move(item));
    }

    std::sort(items_.begin(), items_.end(), [](const CaptureItem& a, const CaptureItem& b) {
        return a.modifiedAt > b.modifiedAt;
    });
}

bool CaptureLibrary::Delete(const CaptureItem& item, std::wstring& error) {
    if (!IsInsideRoot(item.path)) {
        error = L"ZERO refused to delete a file outside the capture library.";
        return false;
    }
    std::error_code ec;
    if (!std::filesystem::exists(item.path, ec)) {
        error = L"The capture no longer exists.";
        Refresh();
        return false;
    }
    if (!std::filesystem::remove(item.path, ec) || ec) {
        error = L"ZERO could not delete the selected capture.";
        return false;
    }
    Refresh();
    return true;
}

bool CaptureLibrary::Open(const CaptureItem& item, std::wstring& error) const {
    if (!IsInsideRoot(item.path)) {
        error = L"ZERO refused to open a file outside the capture library.";
        return false;
    }
    if (!std::filesystem::exists(item.path)) {
        error = L"The capture no longer exists.";
        return false;
    }
    return launchShell(L"open", item.path, nullptr, error);
}

bool CaptureLibrary::Reveal(const CaptureItem& item, std::wstring& error) const {
    if (!IsInsideRoot(item.path)) {
        error = L"ZERO refused to reveal a file outside the capture library.";
        return false;
    }
    if (!std::filesystem::exists(item.path)) {
        error = L"The capture no longer exists.";
        return false;
    }
    const std::wstring parameters = L"/select,\"" + item.path.wstring() + L"\"";
    return launchShell(L"open", L"explorer.exe", parameters.c_str(), error);
}

} // namespace zero
