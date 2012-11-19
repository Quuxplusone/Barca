#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <algorithm>
#include <string>
#include <vector>
#include "AlphaBeta.hh"
#include "Board.h"

static int ab_evaluate(const Board &board)
{
    return board.score();
}

static void ab_apply(Board &board, const Move &move)
{
    board.apply_move(move);
}

void ab_findmoves(const Board &board, std::vector<Move> &allmoves)
{
    allmoves = board.find_all_moves();
}

int ab_findattacker(const Board &board)
{
    return board.attacker;
}

AlphaBeta<Board, Move, int> ab(ab_evaluate, ab_apply, ab_findmoves, ab_findattacker);

Board::Board()
{
    /* Assume that the human player takes South, and moves first. */

    attacker = BLACK;

    white_pieces[0] = Piece(ELEPHANT, 4,0);
    white_pieces[1] = Piece(ELEPHANT, 5,0);
    white_pieces[2] = Piece(LION, 3,1);
    white_pieces[3] = Piece(MOUSE, 4,1);
    white_pieces[4] = Piece(MOUSE, 5,1);
    white_pieces[5] = Piece(LION, 6,1);

    black_pieces[0] = Piece(ELEPHANT, 4,9);
    black_pieces[1] = Piece(ELEPHANT, 5,9);
    black_pieces[2] = Piece(LION, 3,8);
    black_pieces[3] = Piece(MOUSE, 4,8);
    black_pieces[4] = Piece(MOUSE, 5,8);
    black_pieces[5] = Piece(LION, 6,8);
}

bool Board::is_occupied(int x, int y) const
{
    for (int i=0; i < 6; ++i) {
        if (black_pieces[i].x == x && black_pieces[i].y == y) return true;
        if (white_pieces[i].x == x && white_pieces[i].y == y) return true;
    }
    return false;
}

bool Piece::scares(const Piece &prey) const
{
    if (prey.type == MOUSE) return (this->type == LION);
    if (prey.type == LION) return (this->type == ELEPHANT);
    assert(prey.type == ELEPHANT);
    return (this->type == MOUSE);
}

bool Piece::is_adjacent(int ax, int ay) const
{
    return abs(x - ax) <= 1 && abs(y - ay) <= 1;
}

Move::Move(const Piece &p, int x, int y):
    from_x(p.x), from_y(p.y),
    to_x(x), to_y(y),
    was_scared(p.is_scared),
    score(0)
{ }

void Board::maybe_append_move(std::vector<Move> &moves, const Piece &p, int x, int y) const
{
    const Piece (&defenders_pieces)[6] = (attacker == WHITE) ? black_pieces : white_pieces;

    /* Can't move to a place where you're scared,
     * nor to an occupied space. */
    for (int i=0; i < 6; ++i) {
        if (white_pieces[i].at(x,y)) return;
        if (black_pieces[i].at(x,y)) return;
        if (defenders_pieces[i].scares(p) && defenders_pieces[i].is_adjacent(x,y))
            return;
    }

    if (clear_line_to(p, x,y)) {
        moves.push_back(Move(p, x,y));
    }
}
    

std::vector<Move> Board::find_all_moves() const
{
    const Piece (&attackers_pieces)[6] = (attacker == WHITE) ? white_pieces : black_pieces;
    const Piece (&defenders_pieces)[6] = (attacker == WHITE) ? black_pieces : white_pieces;

    std::vector<Move> moves;

    if (this->score() == +9999) {
        /* The current defender has just won the game. */
        return moves;
    }

    for (int x=0; x < 10; ++x) {
        for (int y=0; y < 10; ++y) {
            for (int i=0; i < 6; ++i) {
                maybe_append_move(moves, attackers_pieces[i], x, y);
            }
        }
    }
    
    /* If any scared piece can move, then *only* a scared piece may move. */
    bool a_scared_piece_can_move = false;
    for (int i=0; i < (int)moves.size(); ++i) {
        if (moves[i].was_scared) {
            a_scared_piece_can_move = true;
        }
    }
    
    if (a_scared_piece_can_move) {
        int n = moves.size();
        for (int i=0; i < n; ++i) {
            while (i != n && !moves[i].was_scared) {
                moves[i] = moves[--n];
            }
        }
        moves.resize(n);
    }
    
    assert(moves.size() >= 1);
    return moves;
}

void Board::update_scaredness()
{
    for (int i=0; i < 6; ++i) {
        white_pieces[i].is_scared = false;
        black_pieces[i].is_scared = false;
    }
    for (int i=0; i < 6; ++i) {
        Piece &yours = black_pieces[i];
        for (int j=0; j < 6; ++j) {
            Piece &mine = white_pieces[j];
            if (!yours.is_adjacent(mine.x, mine.y)) continue;
            if (yours.scares(mine)) {
                mine.is_scared = true;
            } else if (mine.scares(yours)) {
                yours.is_scared = true;
            }
        }
    }
}

void Board::apply_move(const Move &move)
{
    Piece (&attackers_pieces)[6] = (attacker == WHITE) ? white_pieces : black_pieces;
 
    /* Which piece is the one that's moving? */
    Piece *p = NULL;
    for (int i=0; i < 6; ++i) {
        if (attackers_pieces[i].x == move.from_x && attackers_pieces[i].y == move.from_y) {
            attackers_pieces[i].x = move.to_x;
            attackers_pieces[i].y = move.to_y;
            this->attacker = ((attacker == WHITE) ? BLACK : WHITE);
            this->update_scaredness();
            return;
        }
    }
    /* ONE of the attacker's pieces must be moving! */
    assert(false);
}

bool Board::clear_line_to(const Piece &p, int to_x, int to_y) const
{
    if (p.at(to_x, to_y)) return false;
    int dx = (to_x - p.x);
    int dy = (to_y - p.y);
    if ((dx == 0 || dy == 0) && p.type != LION) {
        /* Rook-like move is okay. */
    } else if ((dx == dy || dx == -dy) && p.type != MOUSE) {
        /* Bishop-like move is okay. */
    } else {
        /* The move is impossible. */
        return false;
    }

    /* Can't move through an occupied space. */
    int ox = p.x;
    int oy = p.y;
    while (true) {
        ox += (dx > 0) - (dx < 0);
        oy += (dy > 0) - (dy < 0);
        if (ox == to_x && oy == to_y) break;
        if (is_occupied(ox, oy)) return false;
    }
    return true;
}

int Board::waterholes_threatened_by(const Piece &p) const
{
    return clear_line_to(p, 3,3) + clear_line_to(p, 3,6) +
           clear_line_to(p, 6,3) + clear_line_to(p, 6,6);
}

/* Return the board's value to the defender.
 * Higher is better for the defender. */
int Board::score() const
{
    int my_score = 0, your_score = 0;
    int my_scared = 0, your_scared = 0;
    int my_threats = 0, your_threats = 0;
    for (int i=0; i < 6; ++i) {
        my_score += white_pieces[i].at(3,3); your_score += black_pieces[i].at(3,3);
        my_score += white_pieces[i].at(3,6); your_score += black_pieces[i].at(3,6);
        my_score += white_pieces[i].at(6,3); your_score += black_pieces[i].at(6,3);
        my_score += white_pieces[i].at(6,6); your_score += black_pieces[i].at(6,6);
        my_scared += white_pieces[i].is_scared;
        your_scared += black_pieces[i].is_scared;
        my_threats += waterholes_threatened_by(white_pieces[i]);
        your_threats += waterholes_threatened_by(black_pieces[i]);
    }
    assert(my_score + your_score <= 4);
    assert(my_score <= 3);
    assert(your_score <= 3);
    
    if (my_score == 3) { assert(attacker == BLACK); return +9999; }
    if (your_score == 3) { assert(attacker == WHITE); return +9999; }

    my_score = 10*my_score + my_threats + your_scared;
    your_score = 10*your_score + your_threats + my_scared;

    int my_advantage = (my_score - your_score);
    
    return (attacker == WHITE) ? -my_advantage : +my_advantage;
}

static int usec_difference(struct timeval a, struct timeval b)
{
    return (b.tv_sec - a.tv_sec) * 1000 * 1000 + ((int)b.tv_usec - (int)a.tv_usec);
}

Move Board::find_best_move() const
{
    Move bestmove;
    int bestvalue;
    int ply = 1;  /* how many plies deep to search */
    std::vector<Move> all_moves = this->find_all_moves();
    printf("Found %d moves\n", (int)all_moves.size());

    /* Spend up to 2 seconds searching. */
    struct timeval start;
    gettimeofday(&start, NULL);
    struct timeval last_iter = start;
    
    for (int ply = 1; ply <= 7; ++ply) {
        if (ply == 2) continue;

        /* Expect the search at level N to take 30 times as long
         * as the search took at level N-1. */
        struct timeval current;
        gettimeofday(&current, NULL);
        int estimated_completion =
            usec_difference(start, current) +
            30*usec_difference(last_iter, current);
        if (estimated_completion > 2*1000*1000) {
            printf("Breaking off search after ply=%d.\n", ply);
            break;
        }

        ab.depth_first_alpha_beta(*this, ply, bestmove, bestvalue,
                                  /*alpha=*/-9999, /*beta=*/+9999);
        if (bestvalue == +9999) break;
    }
    return bestmove;
}

Move Board::find_random_move() const
{
    std::vector<Move> all_moves = this->find_all_moves();
    return all_moves[rand() % all_moves.size()];
}

static char piece2char(Player who, const Piece &p)
{
    if (p.is_scared) return '!';
    switch (p.type)
    {
        case MOUSE: return (who == WHITE ? 'M' : 'm');
        case LION: return (who == WHITE ? 'L' : 'l');
        case ELEPHANT: return (who == WHITE ? 'E' : 'e');
        default: assert(false);
    }
}

std::string Board::str() const
{
    char board[10][10];
    memset(board, '.', sizeof board);
    for (int i=0; i < 6; ++i) {
        board[white_pieces[i].x][white_pieces[i].y] = piece2char(WHITE, white_pieces[i]);
        board[black_pieces[i].x][black_pieces[i].y] = piece2char(BLACK, black_pieces[i]);
    }

    std::string result;
    for (int y=0; y < 10; ++y) {
        for (int x=0; x < 10; ++x) {
            result += board[x][y];
        }
        result += "\n";
    }
    return result;
}
