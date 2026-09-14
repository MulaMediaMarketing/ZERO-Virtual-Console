#pragma once

#include <windows.h>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace zero {

inline std::optional<std::filesystem::path> PathFromUtf8(std::string_view value) {
    if (value.empty()) return std::filesystem::path{};
    if (value.size() > static_cast<size_t>(std::numeric_limits<int>::max())) return std::nullopt;
    const int bytes = static_cast<int>(value.size());
    const int chars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), bytes, nullptr, 0);
    if (chars <= 0) return std::nullopt;
    std::wstring wide(static_cast<size_t>(chars), L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), bytes, wide.data(), chars)) return std::nullopt;
    return std::filesystem::path(std::move(wide));
}

} // namespace zero
