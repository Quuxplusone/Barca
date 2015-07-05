#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <set>
#include <string>
#include "ImageFmtc.h"
#include "Interact.h"
#include "SimplePng.h"

#include "Board.h"
#include "process_image.h"

void get_game(Board &board)
{
    Image img;
    get_screenshot(img);
    board = process_image(img.im, img.w, img.h);
}

static std::set<std::string> seen_it;

int main(int argc, char **argv)
{
    bool play_for[WHITE+1] = {};
    for (int i=1; i < argc; ++i) {
        if (strcmp(argv[i], "--white") == 0) {
            play_for[WHITE] = true;
        } else if (strcmp(argv[i], "--black") == 0) {
            play_for[BLACK] = true;
        } else {
            printf("Invalid option '%s'.\n", argv[i]);
            puts("Valid options are: --black, --white.");
        }
    }
    if (!play_for[BLACK] && !play_for[WHITE]) {
        puts("You didn't specify --black or --white, so I'll just watch this game.\n");
    }

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
        } catch (const opponent_is_thinking&) {
            assert(!play_for[board.attacker]);
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
            if (turns == 0) {
                /* The board just hasn't been cleared since last time. */
                usleep(500*1000);
                continue;
            }
            FILE *fp = fopen("/tmp/barca.log", "a");
            if (play_for[BLACK] == play_for[WHITE]) {
                printf("%s won!\n", (board.attacker == WHITE) ? "White" : "Black");
                fprintf(fp, "%s %3d moves\n", (board.attacker == WHITE) ? "WHITE" : "BLACK", turns);
            } else if (play_for[board.attacker]) {
                puts("I lost!");
                fprintf(fp, "LOST! %3d moves\n", turns);
                assert(!expecting_to_win);
            } else {
                puts("I won!");
                fprintf(fp, "Won   %3d moves\n", turns);
                assert(expecting_to_win);
            }
            fclose(fp);
            /* Click to reset. */
            single_click_at((board.left + board.right)/2, (board.top + board.bottom)/2);
            usleep(1000*1000);
            puts("===============================NEW GAME");
            seen_it.clear();
            turns = 0;
            expecting_to_win = false;
            continue;
        }

        assert(!expecting_to_win);

        if (!play_for[board.attacker]) {
            puts("Attacker is not ME, so I'm going back to sleep for a while.");
            usleep(500*1000);
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

        single_click_at(x,y);
        usleep(100*1000);  /* Let the UI catch up to this click. */

        x = board.left + (board.right - board.left)*(best_move.to_x)/9;
        y = board.top + (board.bottom - board.top)*(best_move.to_y)/9;
        printf("Second click at (%d,%d)\n", x,y);

        single_click_at(x,y);
        usleep(2500*1000);  /* Let the UI catch up to this click. */

        board.apply_move(best_move);
        expecting_to_win = (board.score() == +9999);
    }  /* while */
    return 0;
}
