#include <graphics.h>
#include <iostream>
#include <windows.h>

int main() {
    initgraph(800, 600);

    PIMAGE img = newimage();
    getimage(img, "image/go lift.png");
    putimage(1, 1, img);
    delimage(img);
    getch();
    closegraph();
    return 0;
}