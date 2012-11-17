
#ifndef H_BOARD
 #define H_BOARD

#include <vector>

enum Player {
    ME, YOU
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
};

struct Board {
    Piece my_pieces[6];
    Piece your_pieces[6];
    Player attacker;
    
    int left, right, top, bottom;  /* UI screen measurements */

    Board();

    bool is_occupied(int x, int y) const;
    void print() const;

    std::vector<Move> find_all_moves() const;
    Move find_best_move() const;
    void apply_move(const Move &);
    int score() const;

  private:
    void maybe_append_move(std::vector<Move> &moves, const Piece &p, int x, int y) const;
    void update_scaredness();
};

#endif /* H_BOARD */
