#pragma once

#include <stdlib.h>

struct Image {
    unsigned char (*im)[3];
    int w;
    int h;

    Image() : im(NULL), w(0), h(0) {}
    ~Image() { free(im); }
};

void get_screenshot(Image&);

void single_click();
void single_click_at(int x, int y);
