#include <graphics.h>
#include <string>
#include <windows.h>

int main() {
    initgraph(800, 600);
    wchar_t executablePath[MAX_PATH] = {};
    const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
    std::wstring imagePath(executablePath, pathLength);
    imagePath.resize(imagePath.find_last_of(L"\\/") + 1);
    imagePath += L"image\\go lift.png";
    PIMAGE img = newimage();
    getimage(img, imagePath.c_str());
    putimage(1, 1, img);
    delimage(img);
    getch();
    closegraph();
    return 0;
}