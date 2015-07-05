
#include <stdio.h>
#include <stdlib.h>
#include "Interact.h"
#include "SimplePng.h"

void get_screenshot(Image& img)
{
#ifdef __APPLE__
    system("screencapture /tmp/jorina.png");
#else
    system("import -window root /tmp/jorina.png");
#endif
    int rc = ReadPNG("/tmp/jorina.png", &img.im, &img.w, &img.h);
    if (rc != 0) {
        printf("Error %d reading /tmp/jorina.png!\n", rc);
        exit(1);
    }
}

void single_click()
{
#ifdef __APPLE__
    system("cliclick c:.");
#else
    system("xdotool click 1");
#endif
}

void single_click_at(int x, int y)
{
    char cmd[100];
#ifdef __APPLE__
    sprintf(cmd, "cliclick m:%d,%d c:.", x, y); system(cmd);
#else
    sprintf(cmd, "xdotool mousemove %d %d", x, y); system(cmd);
    system("xdotool click 1");
#endif
}
