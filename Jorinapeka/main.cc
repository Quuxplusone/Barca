#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "Board.h"
#include "ImageFmtc.h"
#include "Interact.h"
#include "process_image.h"

static void get_game(Board &board)
{
    Image img;
    get_screenshot(img);
    board = process_image(img.im, img.w, img.h);
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
    int current_score = 0;
    int current_level = 1;
    int current_moves = 5;
    bool got_cofu = false;
    bool got_furo = false;
    bool got_hapi = false;
    bool got_lemo = false;
    bool could_still_get_lemo_this_level = true;

    while (true) {
        try {
            Board board;
            get_game(board);  /* This might throw. */

            printf("I think my current score is %d.\n", current_score);
            printf("I think level=%d, moves=%d.\n", current_level, current_moves);
            printf("Here's the board I got:\n");
            for (int i=0; i < 7; ++i) {
                for (int j=0; j < 7; ++j) {
                    BoardCircle &bc = board.circles[i][j];
                    char lb = (bc.is_goal_circle ? '[' : ' ');
                    char rb = (bc.is_goal_circle ? ']' : ' ');
                    char ch = (bc.dir == NONE ? ' ' :
                               bc.dir == NORTH ? '^' :
                               bc.dir == SOUTH ? 'v' :
                               bc.dir == EAST ? '>' :
                               bc.dir == WEST ? '<' : '?');
                    printf("%c%c%c", lb, ch, rb);
                }
                printf("\n");
            }

            Move best_move = board.find_best_move(current_moves);

            printf("Best move is (%d,%d)\n", best_move.i, best_move.j);
            int x = board.left + (board.right - board.left)*(best_move.j)/6;
            int y = board.top + (board.bottom - board.top)*(best_move.i)/6;
            printf("Click at (%d,%d)\n", x,y);
            single_click_at(x, y);

            if (best_move.balls_vanished <= 10) {
                current_moves -= 1;
            } else {
                printf("I think I just gained an extra move!\n");
            }
            current_score += best_move.score;

            bool bonus_screen = false;
            if (best_move.goals_left == 0 && !got_lemo && could_still_get_lemo_this_level) {
                got_lemo = true;
                bonus_screen = true;
                current_score += 100;
                printf("LEMO!!\n");
            }
            if (best_move.is_hapi && !got_hapi) {
                got_hapi = true;
                bonus_screen = true;
                current_score += 100;
                printf("HAPI!!\n");
            }
            if (best_move.is_cofu && !got_cofu) {
                got_cofu = true;
                bonus_screen = true;
                current_score += 100;
                printf("COFU!!\n");
            }
            if (best_move.is_furo && !got_furo) {
                got_furo = true;
                bonus_screen = true;
                current_score += 100;
                printf("FURO!!\n");
            }

            /* Sleep while the animation plays.
             * Let's assume it covers 2 balls per second. */
            usleep(best_move.distance_traveled*500*1000);

            if (bonus_screen) {
                usleep(2000*1000);
                single_click();
                usleep(1000*1000);
            }

            if (best_move.goals_left == 0) {
                /* Give some extra time for the level-up screen. */
                usleep(2000*1000);
                int bonus = (current_level+current_moves)*10;
                printf("I think I just got a %d-point level bonus.\n", bonus);
                current_score += bonus;
                current_level += 1;
                current_moves = 5;
                could_still_get_lemo_this_level = true;
            } else {
                /* Wait while the "refill" animation plays. */
                usleep(300*1000);
                could_still_get_lemo_this_level = false;
            }
            consecutive_failures = 0;
        } catch (...) {
            /* Sleep for a second and try again. */
            consecutive_failures += 1;
            if (consecutive_failures == 4) {
                printf("4 consecutive failures to get screenshot!\n");
                printf("Assuming this is the bonus screen.\n");
                single_click();
                usleep(2500*1000);
            }
            if (consecutive_failures == 8) {
                printf("8 consecutive failures to get screenshot!\n");
                exit(1);
            }
            usleep(1000*1000);
        }
    }  /* while */
    return 0;
}
