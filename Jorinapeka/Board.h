#pragma once

#include <vector>

struct Move {
    int i, j;
    int score;
    int balls_vanished;
    int distance_traveled;
    int goals_hit;
    int goals_left;
    bool is_cofu, is_furo, is_hapi;
    int extra_moves_earned;
    bool illegal;

    Move(): i(0), j(0), score(0), balls_vanished(0), distance_traveled(0),
            goals_hit(0), goals_left(0), is_cofu(false), is_furo(false),
            is_hapi(false), extra_moves_earned(0), illegal(false)
    { }
};

enum Direction {
    NONE, NORTH, SOUTH, EAST, WEST
};

struct BoardCircle {
    bool is_goal_circle;
    bool vanished;
    bool unknown;
    Direction dir;
};

struct Board {
    int N;
    std::vector<std::vector<BoardCircle> > circles;
    int top, bottom, left, right;
    int goals_left;
    Move eval_move(int i, int j);
    Move find_best_move(int moves_left) const;
    Move find_goaliest_move() const;
    Move find_longest_move() const;
    void refill();
};
