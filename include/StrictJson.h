#pragma once

#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace zero::json {

struct Value;
using Object = std::map<std::string, Value, std::less<>>;
using Array = std::vector<Value>;

struct Value {
    using Storage = std::variant<std::nullptr_t, bool, int64_t, double, std::string, Array, Object>;
    Storage storage{nullptr};

    bool IsNull() const noexcept { return std::holds_alternative<std::nullptr_t>(storage); }
    bool IsBool() const noexcept { return std::holds_alternative<bool>(storage); }
    bool IsInt() const noexcept { return std::holds_alternative<int64_t>(storage); }
    bool IsDouble() const noexcept { return std::holds_alternative<double>(storage); }
    bool IsNumber() const noexcept { return IsInt() || IsDouble(); }
    bool IsString() const noexcept { return std::holds_alternative<std::string>(storage); }
    bool IsArray() const noexcept { return std::holds_alternative<Array>(storage); }
    bool IsObject() const noexcept { return std::holds_alternative<Object>(storage); }

    const bool* AsBool() const noexcept { return std::get_if<bool>(&storage); }
    const int64_t* AsInt() const noexcept { return std::get_if<int64_t>(&storage); }
    const double* AsDouble() const noexcept { return std::get_if<double>(&storage); }
    const std::string* AsString() const noexcept { return std::get_if<std::string>(&storage); }
    const Array* AsArray() const noexcept { return std::get_if<Array>(&storage); }
    const Object* AsObject() const noexcept { return std::get_if<Object>(&storage); }
};

struct ParseResult {
    Value root;
    std::string error;
    size_t errorOffset{0};
    bool ok{false};
};

class Parser {
public:
    explicit Parser(std::string_view text, size_t maxDepth = 64) noexcept
        : text_(text), maxDepth_(maxDepth) {}

    ParseResult Parse() {
        ParseResult result;
        SkipWhitespace();
        if (!ParseValue(result.root, 0)) {
            result.error = error_;
            result.errorOffset = pos_;
            return result;
        }
        SkipWhitespace();
        if (pos_ != text_.size()) {
            Fail("trailing data after JSON value");
            result.error = error_;
            result.errorOffset = pos_;
            return result;
        }
        result.ok = true;
        return result;
    }

private:
    std::string_view text_;
    size_t pos_{0};
    size_t maxDepth_{64};
    std::string error_;

    void SkipWhitespace() noexcept {
        while (pos_ < text_.size()) {
            const char c = text_[pos_];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') ++pos_;
            else break;
        }
    }

    bool Fail(const char* message) {
        if (error_.empty()) error_ = message;
        return false;
    }

    bool Consume(char expected) noexcept {
        if (pos_ < text_.size() && text_[pos_] == expected) {
            ++pos_;
            return true;
        }
        return false;
    }

    bool Match(std::string_view token) noexcept {
        if (text_.substr(pos_, token.size()) != token) return false;
        pos_ += token.size();
        return true;
    }

    static bool IsHex(char c) noexcept {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    static uint32_t HexValue(char c) noexcept {
        if (c >= '0' && c <= '9') return static_cast<uint32_t>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<uint32_t>(c - 'a' + 10);
        return static_cast<uint32_t>(c - 'A' + 10);
    }

    static void AppendUtf8(uint32_t cp, std::string& out) {
        if (cp <= 0x7F) {
            out.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }

    bool ParseHex4(uint32_t& value) {
        if (pos_ + 4 > text_.size()) return Fail("truncated unicode escape");
        value = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = text_[pos_++];
            if (!IsHex(c)) return Fail("invalid unicode escape");
            value = (value << 4) | HexValue(c);
        }
        return true;
    }

    bool ParseString(std::string& out) {
        if (!Consume('"')) return Fail("expected string");
        out.clear();
        while (pos_ < text_.size()) {
            const unsigned char c = static_cast<unsigned char>(text_[pos_++]);
            if (c == '"') return true;
            if (c < 0x20) return Fail("control character in string");
            if (c != '\\') {
                out.push_back(static_cast<char>(c));
                continue;
            }
            if (pos_ >= text_.size()) return Fail("truncated escape sequence");
            const char esc = text_[pos_++];
            switch (esc) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    uint32_t first = 0;
                    if (!ParseHex4(first)) return false;
                    uint32_t cp = first;
                    if (first >= 0xD800 && first <= 0xDBFF) {
                        if (pos_ + 2 > text_.size() || text_[pos_] != '\\' || text_[pos_ + 1] != 'u')
                            return Fail("high surrogate without low surrogate");
                        pos_ += 2;
                        uint32_t second = 0;
                        if (!ParseHex4(second)) return false;
                        if (second < 0xDC00 || second > 0xDFFF) return Fail("invalid low surrogate");
                        cp = 0x10000 + ((first - 0xD800) << 10) + (second - 0xDC00);
                    } else if (first >= 0xDC00 && first <= 0xDFFF) {
                        return Fail("unexpected low surrogate");
                    }
                    AppendUtf8(cp, out);
                    break;
                }
                default: return Fail("invalid escape sequence");
            }
        }
        return Fail("unterminated string");
    }

    bool ParseNumber(Value& out) {
        const size_t start = pos_;
        if (Consume('-')) {
            if (pos_ >= text_.size()) return Fail("invalid number");
        }
        if (Consume('0')) {
            if (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9')
                return Fail("leading zero in number");
        } else {
            if (pos_ >= text_.size() || text_[pos_] < '1' || text_[pos_] > '9') return Fail("invalid number");
            while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') ++pos_;
        }

        bool floating = false;
        if (Consume('.')) {
            floating = true;
            const size_t fractionStart = pos_;
            while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') ++pos_;
            if (pos_ == fractionStart) return Fail("fraction requires digits");
        }
        if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
            floating = true;
            ++pos_;
            if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) ++pos_;
            const size_t exponentStart = pos_;
            while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') ++pos_;
            if (pos_ == exponentStart) return Fail("exponent requires digits");
        }

        const auto token = text_.substr(start, pos_ - start);
        if (!floating) {
            int64_t value = 0;
            const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
            if (parsed.ec == std::errc{} && parsed.ptr == token.data() + token.size()) {
                out.storage = value;
                return true;
            }
        }

        std::string copy(token);
        char* end = nullptr;
        errno = 0;
        const double value = std::strtod(copy.c_str(), &end);
        if (errno == ERANGE || !end || *end != '\0' || !std::isfinite(value)) return Fail("number out of range");
        out.storage = value;
        return true;
    }

    bool ParseArray(Value& out, size_t depth) {
        if (!Consume('[')) return Fail("expected array");
        Array array;
        SkipWhitespace();
        if (Consume(']')) { out.storage = std::move(array); return true; }
        for (;;) {
            Value item;
            SkipWhitespace();
            if (!ParseValue(item, depth + 1)) return false;
            array.push_back(std::move(item));
            SkipWhitespace();
            if (Consume(']')) break;
            if (!Consume(',')) return Fail("expected comma in array");
            SkipWhitespace();
        }
        out.storage = std::move(array);
        return true;
    }

    bool ParseObject(Value& out, size_t depth) {
        if (!Consume('{')) return Fail("expected object");
        Object object;
        SkipWhitespace();
        if (Consume('}')) { out.storage = std::move(object); return true; }
        for (;;) {
            SkipWhitespace();
            std::string key;
            if (!ParseString(key)) return false;
            SkipWhitespace();
            if (!Consume(':')) return Fail("expected colon after object key");
            SkipWhitespace();
            Value value;
            if (!ParseValue(value, depth + 1)) return false;
            if (!object.emplace(std::move(key), std::move(value)).second)
                return Fail("duplicate object key");
            SkipWhitespace();
            if (Consume('}')) break;
            if (!Consume(',')) return Fail("expected comma in object");
            SkipWhitespace();
        }
        out.storage = std::move(object);
        return true;
    }

    bool ParseValue(Value& out, size_t depth) {
        if (depth > maxDepth_) return Fail("maximum JSON nesting depth exceeded");
        SkipWhitespace();
        if (pos_ >= text_.size()) return Fail("expected JSON value");
        switch (text_[pos_]) {
            case '{': return ParseObject(out, depth);
            case '[': return ParseArray(out, depth);
            case '"': {
                std::string value;
                if (!ParseString(value)) return false;
                out.storage = std::move(value);
                return true;
            }
            case 't': if (Match("true")) { out.storage = true; return true; } break;
            case 'f': if (Match("false")) { out.storage = false; return true; } break;
            case 'n': if (Match("null")) { out.storage = nullptr; return true; } break;
            default:
                if (text_[pos_] == '-' || (text_[pos_] >= '0' && text_[pos_] <= '9')) return ParseNumber(out);
                break;
        }
        return Fail("invalid JSON value");
    }
};

inline ParseResult Parse(std::string_view text, size_t maxDepth = 64) {
    return Parser(text, maxDepth).Parse();
}

inline const Value* Find(const Object& object, std::string_view key) noexcept {
    const auto it = object.find(key);
    return it == object.end() ? nullptr : &it->second;
}

inline const std::string* String(const Object& object, std::string_view key) noexcept {
    const auto* value = Find(object, key);
    return value ? value->AsString() : nullptr;
}

inline std::optional<int64_t> Integer(const Object& object, std::string_view key) noexcept {
    const auto* value = Find(object, key);
    if (!value) return std::nullopt;
    if (const auto* integer = value->AsInt()) return *integer;
    return std::nullopt;
}

inline std::optional<bool> Boolean(const Object& object, std::string_view key) noexcept {
    const auto* value = Find(object, key);
    if (!value) return std::nullopt;
    if (const auto* boolean = value->AsBool()) return *boolean;
    return std::nullopt;
}

} // namespace zero::json
