#include "StrictJson.h"
#include <iostream>
#include <string>

using zero::json::Parse;

namespace {
bool expect(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; return false; }
    return true;
}
}

int main() {
    bool ok = true;
    auto valid = Parse(R"({"schema":1,"name":"ZERO","enabled":true,"n":4})");
    ok &= expect(valid.ok && valid.root.AsObject(), "valid object must parse");
    ok &= expect(!Parse(R"({"package_id":"a","package_id":"b"})").ok, "duplicate keys must be rejected");
    ok &= expect(!Parse(R"({"schema":1} garbage)").ok, "trailing data must be rejected");
    ok &= expect(!Parse(R"({"x":"\q"})").ok, "unknown escape must be rejected");
    ok &= expect(!Parse(R"({"x":"\uD800"})").ok, "lone high surrogate must be rejected");
    ok &= expect(!Parse(R"({"x":"\uDC00"})").ok, "lone low surrogate must be rejected");

    std::string invalidUtf8 = "{\"x\":\"";
    const unsigned char invalidBytes[] = {0xC0u, 0xAFu};
    invalidUtf8.append(reinterpret_cast<const char*>(invalidBytes), sizeof(invalidBytes));
    invalidUtf8 += "\"}";
    ok &= expect(!Parse(invalidUtf8).ok, "overlong UTF-8 must be rejected");

    ok &= expect(!Parse(R"({"n":01})").ok, "leading-zero number must be rejected");
    ok &= expect(!Parse(R"({"x":[1,2})").ok, "mismatched containers must be rejected");
    std::string deep;
    for (int i = 0; i < 70; ++i) deep += '[';
    deep += '0';
    for (int i = 0; i < 70; ++i) deep += ']';
    ok &= expect(!Parse(deep, 32).ok, "excessive nesting must be rejected");
    ok &= expect(Parse(R"({"x":"\uD83D\uDE80"})").ok, "valid surrogate pair must parse");
    if (!ok) return 1;
    std::cout << "PASS: ZERO strict JSON hostile-input contract\n";
    return 0;
}
