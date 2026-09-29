#include <graphics.h>
#include <string>
#include <windows.h>

int graphx = 800, graphy = 600;

int main() {
    initgraph(graphx, graphy);
    wchar_t executablePath[MAX_PATH] = {};
    const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
    std::wstring imagePath(executablePath, pathLength);
    imagePath.resize(imagePath.find_last_of(L"\\/") + 1);
    imagePath += L"image\\stand.png";
    PIMAGE img = newimage();
    getimage(img, imagePath.c_str());
    putimage(100, 100, img);
    int x = 1, y = 1;
    while (!kbhit()) {
        char ch = getch();
        if (ch == 'd') {
            if (y == graphy) continue;
            y--;
        } else if (ch == 'a') {
            if (y == 1) continue;
            y++;
        } else if (ch == 'w') {
            if (x == 1) continue;
            x--;
        } else if (ch == 's') {
            if (x == graphx) continue;
            x--;
        }
        putimage(x, y, img);
    }
    delimage(img);
    getch();
    closegraph();
    return 0;
}