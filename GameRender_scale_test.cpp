// 界面映射的纯逻辑测试：把 GameRenderImpl.h 里那套整数公式原样抄出来，穷举窗口尺寸与缩放档位，
// 验证「设计坐标 1600×900 排的东西一定能完整落在窗口内」，并且鼠标反算与绘制互逆。
// 只做数学验证，不创建窗口、不碰 GUI；被 CMake 的 _test.cpp 排除规则挡在主程序之外。
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>
#include <vector>

namespace {

constexpr int DesignWidth = 1600;
constexpr int DesignHeight = 900;

int windowWidth = 1200;
int windowHeight = 675;
int uiScalePercent = 120;

// ---- 与 GameRenderImpl.h 完全一致的映射实现 ----
int uiFitPercent() {
    return std::min(windowWidth * 100 / DesignWidth, windowHeight * 100 / DesignHeight);
}

int uiOffsetX() { return (windowWidth - DesignWidth * uiFitPercent() / 100) / 2; }

int uiOffsetY() { return (windowHeight - DesignHeight * uiFitPercent() / 100) / 2; }

int scaledX(int value) {
    return uiOffsetX() + static_cast<int>(static_cast<long long>(value) * uiFitPercent() / 100);
}

int scaledY(int value) {
    return uiOffsetY() + static_cast<int>(static_cast<long long>(value) * uiFitPercent() / 100);
}

bool mouseDesignPos(int px, int py, int& dx, int& dy) {
    const int fit = uiFitPercent();
    const int originX = uiOffsetX();
    const int originY = uiOffsetY();
    if (fit <= 0 || px < originX || py < originY) return false;
    dx = static_cast<int>(static_cast<long long>(px - originX) * 100 / fit);
    dy = static_cast<int>(static_cast<long long>(py - originY) * 100 / fit);
    return true;
}
// ---- 映射实现结束 ----

struct Rect {
    int left, top, right, bottom;
    const char* name;
};

// 各页面里最靠右、最靠下的按钮，只要这些完整可见，其余都在里面
std::vector<Rect> layoutRects() {
    std::vector<Rect> rects;
    for (int i = 0; i < 5; ++i)
        rects.push_back({820, 268 + i * 88, 1160, 334 + i * 88, "主菜单右列"});
    for (int i = 0; i < 3; ++i)
        rects.push_back({450 + i * 320, 760, 700 + i * 320, 830, "主菜单底部一排"});
    rects.push_back({1420, 140, 1560, 200, "语言按钮"});
    for (int i = 0; i < 3; ++i) {
        rects.push_back({130, 215 + i * 100, 1470, 265 + i * 100, "设置行"});
        rects.push_back({1330, 215 + i * 100, 1450, 265 + i * 100, "设置加号"});
    }
    rects.push_back({1150, 540, 1420, 600, "恢复默认"});
    rects.push_back({620, 540, 940, 600, "检查更新"});
    rects.push_back({130, 620, 420, 690, "设置返回"});
    rects.push_back({155, 240, 1440, 296, "商店首行"});
    rects.push_back({155, 700, 430, 766, "商店返回"});
    rects.push_back({150, 655, 570, 720, "帮助翻页"});
    rects.push_back({620, 460, 1080, 530, "暂停按钮"});
    rects.push_back({180, 520, 980, 590, "结算返回"});
    rects.push_back({180, 600, 980, 750, "大厅按钮"});
    return rects;
}

struct Case {
    int width, height, scale;
};

int failures = 0;
int checks = 0;

void expect(bool condition, const char* what, const Case& testCase, const char* detail = "") {
    ++checks;
    if (condition) return;
    ++failures;
    std::printf("FAIL  %-14s  %dx%d scale=%d%%  %s\n", what, testCase.width, testCase.height,
                testCase.scale, detail);
}

void runCase(const Case& testCase) {
    windowWidth = testCase.width;
    windowHeight = testCase.height;
    uiScalePercent = testCase.scale;

    // 1) 设计画布四角必须落在窗口内（这是「右下角按钮被切」的直接判据）
    expect(scaledX(0) >= 0, "画布左边界", testCase);
    expect(scaledY(0) >= 0, "画布上边界", testCase);
    expect(scaledX(DesignWidth) <= testCase.width, "画布右边界", testCase);
    expect(scaledY(DesignHeight) <= testCase.height, "画布下边界", testCase);

    // 2) 等比：每设计单位对应的像素必须一致（窗口比例不对时不能把按钮拉宽压扁）
    const int spanX = scaledX(DesignWidth) - scaledX(0);
    const int spanY = scaledY(DesignHeight) - scaledY(0);
    expect(std::abs(static_cast<long long>(spanX) * DesignHeight -
                    static_cast<long long>(spanY) * DesignWidth) <= 2600,
           "等比拉伸", testCase);

    // 反算的整数精度：窄窗口下 1 像素对应的设计单位更多，容差按 fit 现算
    const int tolerance = 100 / uiFitPercent() + 2;

    // 3) 布局里的每个按钮矩形都必须完整可见
    for (const Rect& rect : layoutRects()) {
        char detail[96];
        std::snprintf(detail, sizeof(detail), "%s [%d,%d,%d,%d]", rect.name, rect.left, rect.top,
                      rect.right, rect.bottom);
        expect(scaledX(rect.left) >= 0 && scaledX(rect.right) <= testCase.width,
               "按钮横向完整", testCase, detail);
        expect(scaledY(rect.top) >= 0 && scaledY(rect.bottom) <= testCase.height,
               "按钮纵向完整", testCase, detail);

        // 4) 画出来的像素点必须反算回同一个设计坐标（鼠标命中与绘制同源）
        int dx = 0, dy = 0;
        const bool inside = mouseDesignPos(scaledX(rect.right), scaledY(rect.bottom), dx, dy);
        expect(inside, "反算落在内容区", testCase, detail);
        expect(std::abs(dx - rect.right) <= tolerance && std::abs(dy - rect.bottom) <= tolerance,
               "反算互逆", testCase, detail);
        const bool centerInside =
            mouseDesignPos((scaledX(rect.left) + scaledX(rect.right)) / 2,
                           (scaledY(rect.top) + scaledY(rect.bottom)) / 2, dx, dy);
        const bool centerHits = centerInside && dx >= rect.left && dx <= rect.right &&
                                dy >= rect.top && dy <= rect.bottom;
        expect(centerHits, "按钮中心可点", testCase, detail);
    }

    // 5) 窗口外侧的点不能被算成设计坐标（否则留边区域会误触发左侧按钮）
    int dx = 0, dy = 0;
    if (uiOffsetX() > 0)
        expect(!mouseDesignPos(uiOffsetX() - 1, windowHeight / 2, dx, dy), "左侧留边不命中",
               testCase);
    if (uiOffsetY() > 0)
        expect(!mouseDesignPos(windowWidth / 2, uiOffsetY() - 1, dx, dy), "上边留边不命中",
               testCase);
}

}  // namespace

int main() {
    const std::vector<std::pair<int, int>> windowSizes = {
        {800, 600},   {1024, 768},  {1200, 675}, {1280, 720},  {1366, 768},
        {1440, 900},  {1600, 900},  {1800, 945}, {1920, 1080}, {2560, 1440},
        {3840, 2160}, {1800, 700},  {2000, 800}, {900, 1200},  {1200, 900},
    };
    const std::vector<int> scales = {100, 110, 120, 130};

    for (const auto& size : windowSizes)
        for (const int scale : scales) runCase({size.first, size.second, scale});

    // 两个真实场景的数字，人工核对用
    windowWidth = 1200;
    windowHeight = 675;
    uiScalePercent = 120;
    std::printf("默认 1200x675 scale=120%%  fit=%d%%  offset=(%d,%d)  底部按钮下沿 y=%d / 窗口 %d\n",
                uiFitPercent(), uiOffsetX(), uiOffsetY(), scaledY(830), windowHeight);

    windowWidth = 1800;
    windowHeight = 945;
    uiScalePercent = 110;
    std::printf("用户 1800x945 scale=110%%  fit=%d%%  offset=(%d,%d)  最右 1470 -> x=%d / 窗口 %d，"
                "最下 830 -> y=%d / 窗口 %d\n",
                uiFitPercent(), uiOffsetX(), uiOffsetY(), scaledX(1470), windowWidth, scaledY(830),
                windowHeight);

    std::printf("%s  checks=%d failures=%d\n", failures == 0 ? "ALL PASSED" : "FAILED", checks,
                failures);
    return failures == 0 ? 0 : 1;
}
