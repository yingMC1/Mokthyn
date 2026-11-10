// 版本更新检查的纯逻辑自测：只验证解析与比较，不联网、不开窗口
#include "game.h/GameUpdateImpl.h"
#include <cstdio>
#include <string>

namespace {
int failures = 0;

void expect(bool condition, const char* label) {
    if (condition) {
        std::printf("ok   %s\n", label);
    } else {
        std::printf("FAIL %s\n", label);
        ++failures;
    }
}

void expectVersion(const std::string& raw, const std::string& expected, const char* label) {
    const std::string actual = mokthyn::updateDetail::locateVersion(raw);
    expect(actual == expected, label);
    if (actual != expected)
        std::printf("     raw=[%s] got=[%s] want=[%s]\n", raw.c_str(), actual.c_str(),
                    expected.c_str());
}
} // namespace

int main() {
    using mokthyn::updateDetail::isVersionLike;
    using mokthyn::updateDetail::newerThanCurrent;

    // 1. 正常响应
    expectVersion("0.1.4", "0.1.4", "plain version");
    expectVersion("  0.1.8\r\n", "0.1.8", "trimmed crlf");
    expectVersion("\xEF\xBB\xBF" "0.1.4", "0.1.4", "utf8 bom prefix");
    expectVersion("version: 0.1.8\n", "0.1.8", "short prefix text");

    // 2. 该拒的必须拒：日期样本、空响应、长 HTML（主机插的验证页）
    expectVersion("2026.11.07", "", "reject date-like");
    expectVersion("", "", "reject empty");
    expectVersion("   \r\n\t ", "", "reject whitespace only");
    expectVersion(std::string(300, 'x') + "0.1.8", "", "reject long html");

    // 3. 格式校验
    expect(isVersionLike("0.1.4"), "accept x.y.z");
    expect(!isVersionLike("0.1.4.2"), "reject four segments");
    expect(!isVersionLike("0.1.4a"), "reject trailing letter");
    expect(!isVersionLike("0.1234.4"), "reject overlong segment");

    // 4. 跨版本比较：当前是 0.1.4，直接跳到 0.1.8 也要认出来
    expect(newerThanCurrent("0.1.8"), "jump 0.1.4 -> 0.1.8 detected");
    expect(newerThanCurrent("0.2.0"), "minor bump detected");
    expect(newerThanCurrent("1.0.0"), "major bump detected");
    expect(!newerThanCurrent("0.1.4"), "same version not newer");
    expect(!newerThanCurrent("0.1.3"), "older version not newer");
    expect(!newerThanCurrent("0.1.4.0"), "four segments not newer");
    expect(!newerThanCurrent(""), "empty not newer");

    // 5. 打开请求标志在启动时必须是未置位的
    expect(!mokthyn::consumeUpdateOpenRequest(), "no pending open request at start");

    if (failures == 0) {
        std::printf("ALL PASSED\n");
        return 0;
    }
    std::printf("%d FAILED\n", failures);
    return 1;
}
