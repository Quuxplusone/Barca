#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "Board.h"
#include "ImageFmtc.h"
#include "Interact.h"
#include "process_image.h"

int average(int a, int b)
{
    return (a + b) / 2;
}

static Board get_game()
{
    Image img;
    get_screenshot(img);
    for (int attempts = 0; attempts < 3; ++attempts) {
        try {
            return process_image(img.im, img.w, img.h);
        } catch (...) { }
        Image img2;
        get_screenshot(img2);
        assert(img.w == img2.w && img.h == img2.h);
        for (int i=0; i < img.w*img.h; ++i) {
            for (int rgb = 0; rgb < 3; ++rgb) {
                img.im[i][rgb] = average(img.im[i][rgb], img2.im[i][rgb]);
            }
        }
    }
    return process_image(img.im, img.w, img.h);
}

int main()
{
    printf("3...\n");
    usleep(1000*1000);
    printf("2...\n");
    usleep(1000*1000);
    printf("1...\n");
    usleep(1000*1000);

    int consecutive_failures = 0;

    while (true) {
        try {
            Board board = get_game();  /* This might throw. */

            printf("Here's the board I got:\n");
            board.print();

            Move best_move = board.find_best_move();

            printf("Best move is (%d,%d) <-> (%d,%d) with %d moves visibly remaining\n",
                   best_move.i, best_move.j, best_move.ti(), best_move.tj(), best_move.moves_visible);
            int x = board.left + (board.right - board.left)*(best_move.j)/(board.N-1);
            int y = board.top + (board.bottom - board.top)*(best_move.i)/(board.N-1);
            printf("First click at (%d,%d)\n", x,y);
            single_click_at(x, y);

            usleep(10*1000);

            x = board.left + (board.right - board.left)*(best_move.tj())/(board.N-1);
            y = board.top + (board.bottom - board.top)*(best_move.ti())/(board.N-1);
            printf("Second click at (%d,%d)\n", x,y);
            single_click_at(x, y);

            x = board.left - (board.right - board.left)*2/(board.N-1);
            y = board.top + (board.bottom - board.top)*3/(board.N-1);
            printf("Dummy click at (%d,%d)\n", x,y);
            single_click_at(x, y);

            /* Sleep while the animation plays.
             * Let's assume it takes half a second to refill the screen. */
            assert(best_move.how_many_refills >= (best_move.illegal ? 0 : 1));
            usleep((best_move.how_many_refills + 1.3)*500*1000);

            consecutive_failures = 0;
        } catch (...) {
            /* Sleep for a second and try again. */
            consecutive_failures += 1;
            if (consecutive_failures == 9) {
                printf("9 consecutive failures to get screenshot!\n");
                exit(1);
            }
            if (consecutive_failures <= 3) {
                usleep(500*1000);
            } else {
                usleep(1500*1000);
            }
        }
    }  /* while */
    return 0;
}
