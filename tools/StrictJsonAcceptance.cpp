#include "StrictJson.h"
#include <iostream>
#include <string>

using zero::json::Parse;

namespace {
bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}
}

int main() {
    bool ok = true;

    auto valid = Parse(R"({"schema":1,"name":"ZERO","enabled":true,"n":4})");
    ok &= expect(valid.ok && valid.root.AsObject(), "valid object must parse");

    auto duplicate = Parse(R"({"package_id":"a","package_id":"b"})");
    ok &= expect(!duplicate.ok, "duplicate keys must be rejected");

    auto trailing = Parse(R"({"schema":1} garbage)");
    ok &= expect(!trailing.ok, "trailing data must be rejected");

    auto badEscape = Parse(R"({"x":"\q"})");
    ok &= expect(!badEscape.ok, "unknown escape must be rejected");

    auto loneHigh = Parse(R"({"x":"\uD800"})");
    ok &= expect(!loneHigh.ok, "lone high surrogate must be rejected");

    auto loneLow = Parse(R"({"x":"\uDC00"})");
    ok &= expect(!loneLow.ok, "lone low surrogate must be rejected");

    const std::string invalidUtf8 = std::string("{\"x\":\"") + char(0xC0) + char(0xAF) + "\"}";
    auto badUtf8 = Parse(invalidUtf8);
    ok &= expect(!badUtf8.ok, "overlong UTF-8 must be rejected");

    auto leadingZero = Parse(R"({"n":01})");
    ok &= expect(!leadingZero.ok, "leading-zero number must be rejected");

    auto wrongClose = Parse(R"({"x":[1,2})");
    ok &= expect(!wrongClose.ok, "mismatched containers must be rejected");

    std::string deep;
    for (int i = 0; i < 70; ++i) deep += '[';
    deep += '0';
    for (int i = 0; i < 70; ++i) deep += ']';
    auto tooDeep = Parse(deep, 32);
    ok &= expect(!tooDeep.ok, "excessive nesting must be rejected");

    auto surrogatePair = Parse(R"({"x":"\uD83D\uDE80"})");
    ok &= expect(surrogatePair.ok, "valid surrogate pair must parse");

    if (!ok) return 1;
    std::cout << "PASS: ZERO strict JSON hostile-input contract\n";
    return 0;
}
