#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
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

    attacker = YOU;

    my_pieces[0] = Piece(ELEPHANT, 4,0);
    my_pieces[1] = Piece(ELEPHANT, 5,0);
    my_pieces[2] = Piece(LION, 3,1);
    my_pieces[3] = Piece(MOUSE, 4,1);
    my_pieces[4] = Piece(MOUSE, 5,1);
    my_pieces[5] = Piece(LION, 6,1);

    your_pieces[0] = Piece(ELEPHANT, 4,9);
    your_pieces[1] = Piece(ELEPHANT, 5,9);
    your_pieces[2] = Piece(LION, 3,8);
    your_pieces[3] = Piece(MOUSE, 4,8);
    your_pieces[4] = Piece(MOUSE, 5,8);
    your_pieces[5] = Piece(LION, 6,8);
}

bool Board::is_occupied(int x, int y) const
{
    for (int i=0; i < 6; ++i) {
        if (your_pieces[i].x == x && your_pieces[i].y == y) return true;
        if (my_pieces[i].x == x && my_pieces[i].y == y) return true;
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
    /* Can't move to an occupied space. */
    if (is_occupied(x,y)) return;
    
    const Piece (&defenders_pieces)[6] = (attacker == ME) ? your_pieces : my_pieces;

    /* Can't move to a place where you're scared. */
    for (int i=0; i < 6; ++i) {
        if (defenders_pieces[i].scares(p) && defenders_pieces[i].is_adjacent(x,y))
            return;
    }

    Move m(p, x, y);
    int dx = (m.from_x - m.to_x);
    int dy = (m.from_y - m.to_y);
    if ((dx == 0 || dy == 0) && p.type != LION) {
        /* Rook-like move is okay. */
    } else if ((dx == dy || dx == -dy) && p.type != MOUSE) {
        /* Bishop-like move is okay. */
    } else {
        /* The move is impossible. */
        return;
    }
    
    moves.push_back(m);
}
    

std::vector<Move> Board::find_all_moves() const
{
    const Piece (&attackers_pieces)[6] = (attacker == ME) ? my_pieces : your_pieces;
    const Piece (&defenders_pieces)[6] = (attacker == ME) ? your_pieces : my_pieces;

    std::vector<Move> moves;
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
        my_pieces[i].is_scared = false;
        your_pieces[i].is_scared = false;
    }
    for (int i=0; i < 6; ++i) {
        Piece &yours = your_pieces[i];
        for (int j=0; j < 6; ++j) {
            Piece &mine = my_pieces[j];
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
    Piece (&attackers_pieces)[6] = (attacker == ME) ? my_pieces : your_pieces;
 
    /* Which piece is the one that's moving? */
    Piece *p = NULL;
    for (int i=0; i < 6; ++i) {
        if (attackers_pieces[i].x == move.from_x && attackers_pieces[i].y == move.from_y) {
            attackers_pieces[i].x = move.to_x;
            attackers_pieces[i].y = move.to_y;
            this->attacker = ((attacker == ME) ? YOU : ME);
            this->update_scaredness();
            return;
        }
    }
    /* ONE of the attacker's pieces must be moving! */
    assert(false);
}

/* Return the board's value to the defender.
 * Higher is better for the defender. */
int Board::score() const
{
    int my_score = 0;
    int your_score = 0;
    for (int i=0; i < 6; ++i) {
        my_score += my_pieces[i].at(3,3); your_score += your_pieces[i].at(3,3);
        my_score += my_pieces[i].at(3,6); your_score += your_pieces[i].at(3,6);
        my_score += my_pieces[i].at(6,3); your_score += your_pieces[i].at(6,3);
        my_score += my_pieces[i].at(6,6); your_score += your_pieces[i].at(6,6);
    }
    assert(my_score + your_score <= 4);
    assert(my_score <= 3);
    assert(your_score <= 3);

    int my_advantage =
        (my_score == 3) ? +9999 :
        (your_score == 3) ? -9999 :
        (my_score - your_score);

    return (attacker == ME) ? -my_advantage : +my_advantage;
}

Move Board::find_best_move() const
{
    Move move;
    int value;
    const int ply = 4;  /* how many plies deep to search */

    ab.depth_first_alpha_beta(*this, ply, move, value, /*alpha=*/-9999, /*beta=*/+9999);
    return move;
}

static char piece2char(Player who, Species type)
{
    switch (type)
    {
        case MOUSE: return (who == ME ? 'M' : 'm');
        case LION: return (who == ME ? 'L' : 'l');
        case ELEPHANT: return (who == ME ? 'E' : 'e');
        default: assert(false);
    }
}

void Board::print() const
{
    char board[10][10];
    memset(board, '.', sizeof board);
    for (int i=0; i < 6; ++i) {
        board[my_pieces[i].x][my_pieces[i].y] = piece2char(ME, my_pieces[i].type);
        board[your_pieces[i].x][your_pieces[i].y] = piece2char(YOU, your_pieces[i].type);
    }
    for (int y=0; y < 10; ++y) {
        for (int x=0; x < 10; ++x) {
            printf("%c", board[x][y]);
        }
        printf("\n");
    }
}