// 仅用于验证 GameUpdateImpl.h 里的纯解析/比较逻辑，不会被 CMake 的源码 GLOB 编进主程序
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include "game.h/GameUpdateImpl.h"

namespace {
int failures = 0;

void expectVersion(const char* raw, const char* expected, const char* label) {
    const std::string actual = mokthyn::updateDetail::locateVersion(std::string(raw));
    if (actual != expected) {
        ++failures;
        std::cout << "FAIL " << label << "  raw=[" << raw << "]  got=[" << actual
                  << "]  expect=[" << expected << "]\n";
    } else {
        std::cout << "ok   " << label << "  -> [" << actual << "]\n";
    }
}

void expectNewer(const char* latest, bool expected, const char* label) {
    const bool actual = mokthyn::updateDetail::newerThanCurrent(std::string(latest));
    if (actual != expected) {
        ++failures;
        std::cout << "FAIL " << label << "  latest=[" << latest << "]  got=" << actual
                  << "  expect=" << expected << "\n";
    } else {
        std::cout << "ok   " << label << "  -> " << (actual ? "newer" : "same-or-older") << "\n";
    }
}
} // namespace

int main() {
    expectVersion("0.1.3", "0.1.3", "plain");
    expectVersion("  0.1.4\r\n", "0.1.4", "trim whitespace");
    expectVersion("\xEF\xBB\xBF" "0.1.5", "0.1.5", "utf8 bom");
    expectVersion("2026.11.07", "", "reject date-like");
    expectVersion("", "", "empty");
    expectVersion("   \r\n\t ", "", "only whitespace");
    expectVersion("ver 0.1.9", "0.1.9", "short text with prefix");

    std::string longHtml = "<html><body>update check<p>2026.11.07</p><p>0.1.6</p>";
    longHtml.append(200, 'x');
    expectVersion(longHtml.c_str(), "", "long html not scanned");

    expectNewer("0.1.4", true, "0.1.4 newer than 0.1.3");
    expectNewer("0.1.3", false, "same version");
    expectNewer("0.0.9", false, "older");
    expectNewer("0.2.0", true, "minor bump");
    expectNewer("1.0.0", true, "major bump");

    std::cout << (failures == 0 ? "ALL PASSED\n" : "FAILURES=" ) << (failures == 0 ? "" : std::to_string(failures).c_str()) << "\n";
    return failures == 0 ? 0 : 1;
}
