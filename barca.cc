#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <set>
#include <string>
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

static std::set<std::string> seen_it;

int main()
{
    printf("3...\n");
    usleep(1000*1000);
    printf("2...\n");
    usleep(1000*1000);
    printf("1...\n");
    usleep(1000*1000);
    
    int failures_in_a_row = 0;
    int turns = 0;
    bool expecting_to_win = false;
    while (true) {
        Board board;
        try {
            get_game(board);  /* This might throw. */
        } catch (const char *err) {
            puts("Failed to get a valid board image.");
            printf("Reason provided was: \"%s\"\n", err);
            if (++failures_in_a_row == 3) {
                assert(false);
            }
            usleep(1000*1000);
            continue;
        } catch (...) {
            /* Nothing else should ever be thrown. */
            assert(false);
        }

        failures_in_a_row = 0;
        printf("Here's the board I got:\n");
        printf("%s", board.str().c_str());

        if (board.score() == +9999) {
            /* Somebody won. */
            FILE *fp = fopen("/tmp/barca.log", "a");
            if (board.attacker == ME) {
                puts("I lost!");
                fprintf(fp, "LOST! %3d moves\n", turns);
                assert(!expecting_to_win);
            } else {
                puts("I won!");
                fprintf(fp, "Won   %3d moves\n", turns);
                assert(expecting_to_win);
            }
            fclose(fp);
            system("xdotool click 1");  /* Click to reset. */
            usleep(1000*1000);
            puts("===============================NEW GAME");
            seen_it.clear();
            turns = 0;
            expecting_to_win = false;
            continue;
        }

        assert(!expecting_to_win);

        if (board.attacker != ME) {
            puts("Attacker is not ME, so I'm going back to sleep for a while.");
            usleep(1000*1000);
            continue;
        }

        Move best_move = board.find_best_move();
        std::string key = board.str();
        if (!seen_it.insert(key).second) {
            /* The key has already been seen in this game! */
            best_move = board.find_random_move();
            printf("Loop detected. Replacing \"best_move\" with a random move...\n");
        }

        printf("Best move is (%d,%d) to (%d,%d)\n",
               best_move.from_x, best_move.from_y,
               best_move.to_x, best_move.to_y);
        turns += 1;

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
        usleep(2500*1000);  /* Let the UI catch up to this click. */
        
        board.apply_move(best_move);
        expecting_to_win = (board.score() == +9999);
    }  /* while */
    return 0;
}
