#include <graphics.h>
#include <iostream>
#include <string>
#include <windows.h>

int main() {
    initgraph(800, 600);

    wchar_t executablePath[MAX_PATH] = {};
    const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
    if (pathLength == 0 || pathLength >= MAX_PATH) {
        MessageBoxW(getHWnd(), L"无法确定程序所在目录。", L"错误", MB_OK | MB_ICONERROR);
        closegraph();
        return -1;
    }

    std::wstring imagePath(executablePath, pathLength);
    const std::wstring::size_type separator = imagePath.find_last_of(L"\\/");
    if (separator == std::wstring::npos) {
        MessageBoxW(getHWnd(), L"无法确定程序所在目录。", L"错误", MB_OK | MB_ICONERROR);
        closegraph();
        return -1;
    }
    imagePath.resize(separator + 1);
    imagePath += L"image\\go lift.png";

    PIMAGE img = newimage();
    if (img == nullptr || getimage(img, imagePath.c_str()) != grOk) {
        MessageBoxW(getHWnd(), L"图片加载失败，请确认程序目录下存在 image\\go lift.png。", L"错误",
                    MB_OK | MB_ICONERROR);
        if (img != nullptr) {
            delimage(img);
        }
        closegraph();
        return -1;
    }

    putimage(1, 1, img);
    delimage(img);
    getch();
    closegraph();
    return 0;
}