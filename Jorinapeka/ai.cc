#include <assert.h>
#include <stdio.h>
#include "Board.h"

Move Board::eval_move(int i, int j)
{
    Move move;

    move.i = i;
    move.j = j;

    /* None of the circles should be vanished at this point. */
    assert(!circles[i][j].vanished);
    if (circles[i][j].unknown || circles[i][j].is_goal_circle ||
            circles[i][j].dir == NONE) {
        move.score = -1;
        move.goals_left = this->goals_left;
        move.illegal = true;
        return move;
    }
    move.score = -1;
    Direction delta = circles[i][j].dir;
    while (0 <= i && i < 7 && 0 <= j && j < 7 &&
           !circles[i][j].unknown) {
        ++move.distance_traveled;
        if (circles[i][j].vanished) {
            /* Don't change direction; don't increase the score. */
        } else {
            if (circles[i][j].dir != NONE)
              delta = circles[i][j].dir;
            if (circles[i][j].is_goal_circle) {
                move.goals_hit += 1;
                move.score += 5 * move.goals_hit;
                circles[i][j].is_goal_circle = false;
                goals_left -= 1;
            } else {
                move.score += 1 + move.goals_hit;
            }
            circles[i][j].dir = NONE;
            circles[i][j].vanished = true;
            ++move.balls_vanished;
        }
        switch (delta) {
            case NORTH: i -= 1; break;
            case SOUTH: i += 1; break;
            case WEST: j -= 1; break;
            case EAST: j += 1; break;
            default: assert(false);
        }
    }

    assert(goals_left >= 0);
    move.goals_left = goals_left;

    move.extra_moves_earned = (move.balls_vanished > 10);
    move.extra_moves_earned += (move.balls_vanished > 20);
    move.extra_moves_earned += (move.balls_vanished > 30);
    move.extra_moves_earned += (move.balls_vanished > 40);

    move.is_hapi = (move.balls_vanished > 49/2);

    /* See if we just cleared a full row or column. */
    for (int k=0; k < 7; ++k) {
        bool cofu = true;
        bool furo = true;
        for (int l=0; l < 7; ++l) {
            if (!circles[k][l].vanished) furo = false;
            if (!circles[l][k].vanished) cofu = false;
        }
        if (cofu) move.is_cofu = true;
        if (furo) move.is_furo = true;
    }
    return move;
}

Move Board::find_longest_move() const
{
    Move bestmove;
    bestmove.illegal = true;
    for (int oj=0; oj < 7; ++oj) {
        for (int oi=0; oi < 7; ++oi) {
            int j = oj, i = oi;
            Board temp = *this;
            Move move = temp.eval_move(oi, oj);
            if (move.illegal) continue;
            if (move.balls_vanished > bestmove.balls_vanished)
              bestmove = move;
        }
    }
    return bestmove;
}

Move Board::find_goaliest_move() const
{
    Move bestmove;
    bestmove.illegal = true;
    bestmove.goals_hit = -1;
    for (int oj=0; oj < 7; ++oj) {
        for (int oi=0; oi < 7; ++oi) {
            int j = oj, i = oi;
            Board temp = *this;
            Move move = temp.eval_move(oi, oj);
            if (move.illegal) continue;
            if (move.goals_hit > bestmove.goals_hit)
              bestmove = move;
        }
    }
    return bestmove;
}

Move Board::find_best_move(int moves_left) const
{
    assert(moves_left >= 1);
    Move bestmove;
    bestmove.illegal = true;
    int bestlen = 0;
    for (int oj=0; oj < 7; ++oj) {
        for (int oi=0; oi < 7; ++oi) {
            int j = oj, i = oi;
            Board temp = *this;
            Move move = temp.eval_move(oi, oj);
            if (move.illegal) continue;
            int curlen = move.score;
            if (moves_left + move.extra_moves_earned == 1) {
                /* The game will end with this move. */
                if (move.goals_left == 0)
                  curlen += 10000;
                else
                  curlen -= 10000;
            } else if (move.goals_left == 0) {
                /* This move would end the current level.
                 * We don't want to allow that unless we have
                 * no other option; if there's a "stalling" move
                 * that would let us come back to this move later,
                 * then take that move instead. */
            } else {
                /* We get to keep playing after this move.
                 * If this is the above-mentioned "stalling" move,
                 * then raise its score. */
                Board temp2 = temp;
                temp2.refill();
                Move longmove = temp2.find_longest_move();
                longmove.illegal = true;
                Move winmove = temp2.find_goaliest_move();
                if (!longmove.illegal && moves_left + move.extra_moves_earned +
                    -2 + longmove.extra_moves_earned >= temp2.goals_left+2) {
                    /* I'm sure this will be plenty of moves.
                     * We shouldn't penalize a high-scoring move
                     * by elevating a short conservative move
                     * over it. */
                    curlen += 1000;
                } else if (!winmove.illegal && winmove.goals_left == 0 &&
                           (move.score >= 10 || move.extra_moves_earned > 0)) {
                    /* We have a winning move on the board! */
                    curlen += 1000;
                }
            }
printf("%d,%d curlen=%d\n", oi, oj, curlen);
            if (curlen > bestlen) {
                bestmove = move;
                bestlen = curlen;
            }
        }
    }
    assert(!bestmove.illegal);
    return bestmove;
}


void Board::refill()
{
    for (int j=0; j < 7; ++j) {
        for (int i=6; i >= 0; --i) {
            if (circles[i][j].vanished) {
                /* Scan up, grab the first unvanished ball, and drag it down. */
                int d = i-1;
                while (d >= 0 && circles[d][j].vanished)
                  --d;
                if (d >= 0) {
                    circles[i][j].dir = circles[d][j].dir;
                    circles[i][j].unknown = circles[d][j].unknown;
                    circles[i][j].vanished = false;
                    circles[d][j].vanished = true;
                } else {
                    circles[i][j].dir = NONE;
                    circles[i][j].unknown = true;
                    circles[i][j].vanished = false;
                }
            }
        }
    }
}
