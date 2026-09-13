#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace zero::protocol {

inline constexpr uint16_t kVersion = 4;
inline constexpr uint32_t kMaxFrameBytes = 64u * 1024u;
inline constexpr uint32_t kMaxFields = 16u;

enum class MessageType : uint16_t {
    Hello = 1,
    Welcome = 2,
    Ready = 3,
    Ack = 4,
    Resume = 5,
    Achievement = 6,
    Overlay = 7,
    OverlayAck = 8,
    Ping = 9,
    Pong = 10,
    Error = 11
};

struct Message {
    MessageType type{MessageType::Error};
    uint64_t requestId{0};
    std::vector<std::string> fields;
};

inline bool IsValidUtf8(const std::string& value) {
    if (value.empty()) return true;
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                               static_cast<int>(value.size()), nullptr, 0) > 0;
}

inline void AppendU16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xff));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
}

inline void AppendU32(std::vector<uint8_t>& out, uint32_t value) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xff));
}

inline void AppendU64(std::vector<uint8_t>& out, uint64_t value) {
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xff));
}

inline bool ReadU16(const std::vector<uint8_t>& in, size_t& offset, uint16_t& value) {
    if (offset + 2 > in.size()) return false;
    value = static_cast<uint16_t>(in[offset]) |
            static_cast<uint16_t>(in[offset + 1] << 8);
    offset += 2;
    return true;
}

inline bool ReadU32(const std::vector<uint8_t>& in, size_t& offset, uint32_t& value) {
    if (offset + 4 > in.size()) return false;
    value = 0;
    for (int i = 0; i < 4; ++i) value |= static_cast<uint32_t>(in[offset + i]) << (i * 8);
    offset += 4;
    return true;
}

inline bool ReadU64(const std::vector<uint8_t>& in, size_t& offset, uint64_t& value) {
    if (offset + 8 > in.size()) return false;
    value = 0;
    for (int i = 0; i < 8; ++i) value |= static_cast<uint64_t>(in[offset + i]) << (i * 8);
    offset += 8;
    return true;
}

inline bool Encode(const Message& message, std::vector<uint8_t>& payload) {
    if (message.fields.size() > kMaxFields) return false;
    payload.clear();
    AppendU16(payload, kVersion);
    AppendU16(payload, static_cast<uint16_t>(message.type));
    AppendU64(payload, message.requestId);
    AppendU32(payload, static_cast<uint32_t>(message.fields.size()));
    for (const auto& field : message.fields) {
        if (!IsValidUtf8(field) || field.size() > kMaxFrameBytes) return false;
        AppendU32(payload, static_cast<uint32_t>(field.size()));
        payload.insert(payload.end(), field.begin(), field.end());
        if (payload.size() > kMaxFrameBytes) return false;
    }
    return payload.size() <= kMaxFrameBytes;
}

inline bool Decode(const std::vector<uint8_t>& payload, Message& message) {
    if (payload.empty() || payload.size() > kMaxFrameBytes) return false;
    size_t offset = 0;
    uint16_t version = 0;
    uint16_t type = 0;
    uint32_t fieldCount = 0;
    if (!ReadU16(payload, offset, version) || version != kVersion) return false;
    if (!ReadU16(payload, offset, type)) return false;
    if (!ReadU64(payload, offset, message.requestId)) return false;
    if (!ReadU32(payload, offset, fieldCount) || fieldCount > kMaxFields) return false;
    message.type = static_cast<MessageType>(type);
    message.fields.clear();
    message.fields.reserve(fieldCount);
    for (uint32_t i = 0; i < fieldCount; ++i) {
        uint32_t length = 0;
        if (!ReadU32(payload, offset, length) || length > kMaxFrameBytes || offset + length > payload.size()) return false;
        std::string field(reinterpret_cast<const char*>(payload.data() + offset), length);
        if (!IsValidUtf8(field)) return false;
        message.fields.push_back(std::move(field));
        offset += length;
    }
    return offset == payload.size();
}

inline bool WriteExact(HANDLE pipe, const void* data, uint32_t bytes) {
    const auto* cursor = static_cast<const uint8_t*>(data);
    uint32_t remaining = bytes;
    while (remaining > 0) {
        DWORD written = 0;
        if (!WriteFile(pipe, cursor, remaining, &written, nullptr) || written == 0) return false;
        cursor += written;
        remaining -= written;
    }
    return true;
}

inline bool ReadExact(HANDLE pipe, void* data, uint32_t bytes) {
    auto* cursor = static_cast<uint8_t*>(data);
    uint32_t remaining = bytes;
    while (remaining > 0) {
        DWORD read = 0;
        if (!ReadFile(pipe, cursor, remaining, &read, nullptr) || read == 0) return false;
        cursor += read;
        remaining -= read;
    }
    return true;
}

inline bool WriteMessage(HANDLE pipe, const Message& message) {
    std::vector<uint8_t> payload;
    if (!Encode(message, payload)) return false;
    uint8_t length[4] = {
        static_cast<uint8_t>(payload.size() & 0xff),
        static_cast<uint8_t>((payload.size() >> 8) & 0xff),
        static_cast<uint8_t>((payload.size() >> 16) & 0xff),
        static_cast<uint8_t>((payload.size() >> 24) & 0xff)
    };
    return WriteExact(pipe, length, 4) &&
           WriteExact(pipe, payload.data(), static_cast<uint32_t>(payload.size()));
}

inline bool ReadMessage(HANDLE pipe, Message& message) {
    uint8_t lengthBytes[4]{};
    if (!ReadExact(pipe, lengthBytes, 4)) return false;
    const uint32_t length = static_cast<uint32_t>(lengthBytes[0]) |
                            (static_cast<uint32_t>(lengthBytes[1]) << 8) |
                            (static_cast<uint32_t>(lengthBytes[2]) << 16) |
                            (static_cast<uint32_t>(lengthBytes[3]) << 24);
    if (length == 0 || length > kMaxFrameBytes) return false;
    std::vector<uint8_t> payload(length);
    return ReadExact(pipe, payload.data(), length) && Decode(payload, message);
}

} // namespace zero::protocol
