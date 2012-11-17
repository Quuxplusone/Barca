#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ImageFmtc.h"
#include "Board.h"

Board process_image(unsigned char (*im)[3], int w, int h);
extern "C" int ReadPNG(const char *fname, unsigned char (**data)[3], int *w, int *h);

void get_game(Board &board)
{
    unsigned char (*im)[3];
    int w, h, rc;
#ifdef __APPLE__
    system("screencapture /tmp/jorina.png");
#else
    system("import -window root /tmp/jorina.png");
#endif
    rc = ReadPNG("/tmp/jorina.png", &im, &w, &h);
    if (rc != 0) {
        printf("Error %d reading /tmp/jorina.png!\n", rc);
        exit(1);
    }
    board = process_image(im, w, h);
    free(im);
}


int main()
{
    while (true) {
        Board board;
        try {
            get_game(board);  /* This might throw. */
        } catch (const char *err) {
            puts("Failed to get a valid board image.");
            printf("Reason provided was: \"%s\"\n", err);
            usleep(1000*1000);
            continue;
        } catch (...) {
            /* Nothing else should ever be thrown. */
            assert(false);
        }

        printf("Here's the board I got:\n");
        board.print();

        if (board.attacker != ME) {
            puts("Attacker is not ME, so I'm going back to sleep for a while.");
            usleep(1000*1000);
            continue;
        }

        Move best_move = board.find_best_move();

        printf("Best move is (%d,%d) to (%d,%d)\n",
               best_move.from_x, best_move.from_y,
               best_move.to_x, best_move.to_y);

        int x = board.left + (board.right - board.left)*(best_move.from_x)/9;
        int y = board.top + (board.bottom - board.top)*(best_move.from_y)/9;
        printf("First click at (%d,%d)\n", x,y);

        char cmd[100];
        sprintf(cmd, "xdotool mousemove %d %d", x, y); system(cmd);
        system("xdotool click 1");
        usleep(100*1000);  /* Let the UI catch up to this click. */

        x = board.left + (board.right - board.left)*(best_move.to_x)/9;
        y = board.top + (board.bottom - board.top)*(best_move.to_y)/9;
        printf("Second click at (%d,%d)\n", x,y);

        sprintf(cmd, "xdotool mousemove %d %d", x, y); system(cmd);
        system("xdotool click 1");
        usleep(2000*1000);  /* Let the UI catch up to this click. */

    }  /* while */
    return 0;
}
