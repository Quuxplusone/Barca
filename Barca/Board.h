#pragma once

#include <string>
#include <vector>

enum Player {
    BLACK, WHITE
};

enum Species {
    MOUSE, LION, ELEPHANT
};

struct Piece {
    Species type;
    int x, y;
    bool is_scared;

    Piece() { }
    Piece(Species s, int x, int y):
        type(s), x(x), y(y), is_scared(false)
    { }

    bool scares(const Piece &prey) const;
    bool is_adjacent(int ax, int ay) const;
    bool at(int ax, int ay) const { return (x==ax && y==ay); }
};

struct Move {
    int from_x, from_y;
    int to_x, to_y;
    bool was_scared;

    int score;

    Move() { }
    Move(const Piece &p, int x, int y);
    std::string str() const;
};

struct Board {
    Piece white_pieces[6];
    Piece black_pieces[6];
    Player attacker;

    int left, right, top, bottom;  /* UI screen measurements */

    Board();

    bool is_occupied(int x, int y) const;
    void update_scaredness();

    std::vector<Move> find_all_moves() const;
    Move find_best_move() const;
    Move find_random_move() const;
    void apply_move(const Move &);
    int score() const;
    std::string str() const;

  private:
    void maybe_append_move(std::vector<Move> &moves, const Piece &p, int x, int y) const;
    void maybe_append_trapped_move(std::vector<Move> &moves, const Piece &p, int x, int y) const;
    bool clear_line_to(const Piece &p, int ax, int ay) const;
    int waterholes_threatened_by(const Piece &p) const;
};
