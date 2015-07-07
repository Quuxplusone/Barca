#include <assert.h>
#include <stdio.h>
#include "Board.h"

int Move::fitness() const
{
    int fitness = 0;
    if (moves_visible >= 1) fitness += 100;
    if (moves_visible >= 2) fitness += 100;
    if (moves_visible >= 3) fitness += 100;
    fitness += moves_visible;
    fitness += -1*star_gems;
    fitness += -1*flame_gems;
    fitness += 10*hypercubes;
    return fitness;
}

static bool same_color(Gem& a, Gem& b)
{
    if (a.unknown || b.unknown) return false;
    if (a.special == HYPERCUBE || b.special == HYPERCUBE) return false;
    return a.color == b.color;
}

bool Board::contains_matches()
{
    bool got_any_matches = false;

    for (int i=0; i < N; ++i) {
        for (int j=0; j < N; ++j) {
            gems[i][j].length_of_horizontal_match = 0;
            gems[i][j].length_of_vertical_match = 0;
        }
    }

    for (int i=0; i < N; ++i) {
        for (int j=0; j < N; ++j) {
            if (gems[i][j].unknown) continue;
            if (i == 0 || !same_color(gems[i][j], gems[i-1][j])) {
                int d = 0;
                while (i+d < N && same_color(gems[i][j], gems[i+d][j]))
                    ++d;
                if (d >= 3) {
                    got_any_matches = true;
                    for (int dd = 0; dd < d; ++dd) {
                        gems[i+dd][j].length_of_vertical_match = d;
                    }
                }
            }
            if (j == 0 || !same_color(gems[i][j], gems[i][j-1])) {
                int d = 0;
                while (j+d < N && same_color(gems[i][j], gems[i][j+d]))
                    ++d;
                if (d >= 3) {
                    got_any_matches = true;
                    for (int dd = 0; dd < d; ++dd) {
                        gems[i][j+dd].length_of_horizontal_match = d;
                    }
                }
            }
        }
    }

    return got_any_matches;
}

void Board::blow_up(int i, int j)
{
    if (i < 0 || N <= i) return;
    if (j < 0 || N <= j) return;
    if (gems[i][j].vanished) return;

    gems[i][j].vanished = true;
    switch (gems[i][j].special) {
        case NONE:
            break;
        case FLAME_GEM:
            for (int a = -1; a <= 1; ++a) {
                for (int b = -1; b <= 1; ++b) {
                    blow_up(i+a, j+b);
                }
            }
            break;
        case STAR_GEM:
            for (int a = -N; a < N; ++a) {
                blow_up(i, j+a);
                blow_up(i+a, j);
            }
            break;
        case HYPERCUBE:
            // What happens if a star gem blows up a hypercube?
            // TODO: learn and fix this.
            break;
    }
}

Move Board::check_move(int i, int j, bool swap_downward)
{
    Move move(i, j, swap_downward);

    if (swap_downward) {
        if (i+1 >= N) {
            move.illegal = true;
            return move;
        }
        std::swap(gems[i][j], gems[i+1][j]);
    } else {
        if (j+1 >= N) {
            move.illegal = true;
            return move;
        }
        std::swap(gems[i][j], gems[i][j+1]);
    }

    move.illegal = !this->contains_matches();
    return move;
}

Move Board::eval_move(int i, int j, bool swap_downward)
{
    Move move = check_move(i, j, swap_downward);

    if (move.illegal) return move;

    std::pair<int,int> source = std::make_pair(i, j);
    std::pair<int,int> target = swap_downward ? std::make_pair(i+1,j) : std::make_pair(i,j+1);

    if (gems[source.first][source.second].special == HYPERCUBE) {
        blow_up(source.first, source.second);
    }

    if (gems[target.first][target.second].special == HYPERCUBE) {
        blow_up(target.first, target.second);
    }

    do {
        // Remove all blown-up gems.
        for (int i=0; i < N; ++i) {
            for (int j=0; j < N; ++j) {
                if (gems[i][j].length_of_horizontal_match || gems[i][j].length_of_vertical_match) {
                    // Blow up this gem.
                    blow_up(i,j);
                }
            }
        }

        // Create flame gems and star gems.
        for (int i=0; i < N; ++i) {
            for (int j=0; j < N; ++j) {
                if (gems[i][j].length_of_vertical_match >= 4 && (i == 0 || !same_color(gems[i-1][j], gems[i][j]))) {
                    if (gems[i][j].length_of_vertical_match == 4) {
                        gems[i][j].vanished = false;
                        gems[i][j].special = FLAME_GEM;
                    } else {
                        gems[i][j].vanished = false;
                        gems[i][j].special = HYPERCUBE;
                    }
                }
                if (std::make_pair(i,j) == source || std::make_pair(i,j) == target) {
                    if (gems[i][j].length_of_horizontal_match == 4) {
                        gems[i][j].vanished = false;
                        gems[i][j].special = FLAME_GEM;
                    } else if (gems[i][j].length_of_horizontal_match == 5) {
                        gems[i][j].vanished = false;
                        gems[i][j].special = HYPERCUBE;
                    }
                }
                if (gems[i][j].length_of_vertical_match >= 3 && gems[i][j].length_of_horizontal_match >= 3) {
                    gems[i][j].vanished = false;
                    gems[i][j].special = STAR_GEM;
                    if (gems[i][j].length_of_vertical_match >= 4) {
                        // The star gem always appears ABOVE the flame gem or hypercube.
                        int up = i;
                        while (gems[up][j].vanished) --up;
                        std::swap(gems[up][j], gems[i][j]);
                    }
                }
                // We don't really know where the flame gem or hypercube will wind up
                // for horizontal matches that didn't move this turn.
                // TODO: learn and fix this.
            }
        }

        move.how_many_refills += 1;
        refill();
        source = std::make_pair(-1,-1);
        target = std::make_pair(-1,-1);
    } while (this->contains_matches());

    // Bookkeeping.
    for (int i=0; i < N; ++i) {
        for (int j=0; j < N; ++j) {
            if (gems[i][j].vanished) {
                move.gems_vanished += 1;
            } else {
                switch (gems[i][j].special) {
                    case NONE: break;
                    case FLAME_GEM: move.flame_gems += 1; break;
                    case STAR_GEM: move.star_gems += 1; break;
                    case HYPERCUBE: move.hypercubes += 1; break;
                }
            }
        }
    }

    move.moves_visible = find_all_moves().size();

    return move;
}

std::vector<Move> Board::find_all_moves() const
{
    std::vector<Move> result;
    for (int i=0; i < N; ++i) {
        for (int j=0; j < N; ++j) {
            for (int k=0; k <= 1; ++k) {
                Board temp = *this;
                Move move = temp.check_move(i, j, (bool)k);
                if (!move.illegal) {
                    result.push_back(move);
                }
            }
        }
    }
    return result;
}

Move Board::find_best_move() const
{
    Move bestmove;
    int best_fitness = 0;
    bestmove.illegal = true;
    for (int i=0; i < N; ++i) {
        for (int j=0; j < N; ++j) {
            for (int k=0; k <= 1; ++k) {
                Board temp = *this;
                Move move = temp.eval_move(i, j, (bool)k);
                if (move.illegal) continue;
                printf("move %d,%d,%d yields %d visible moves %d+%d\n", i,j,k, move.moves_visible, move.star_gems, move.flame_gems);
                int fitness = move.fitness();
                if (fitness > best_fitness) {
                    bestmove = move;
                    best_fitness = fitness;
                }
            }
        }
    }
    if (bestmove.illegal) {
        // Try swapping a hypercube with something, at random.
        int hypercubes_seen = 0;
        for (int i=0; i < N; ++i) {
            for (int j=0; j < N; ++j) {
                if (i == N-1 && j == N-1) continue;
                if (gems[i][j].special == HYPERCUBE) {
                    hypercubes_seen += 1;
                    if (rand() % hypercubes_seen == 0) {
                        bestmove.i = i;
                        bestmove.j = j;
                        if (i+1 < N) bestmove.swap_downward = true;
                    }
                }
            }
        }
    }
    return bestmove;
}

void Board::refill()
{
    for (int j=0; j < N; ++j) {
        for (int i = N-1; i >= 0; --i) {
            if (gems[i][j].vanished) {
                /* Scan up, grab the first unvanished gem, and drag it down. */
                int d = i-1;
                while (d >= 0 && gems[d][j].vanished)
                  --d;
                if (d >= 0) {
                    gems[i][j] = gems[d][j];
                    gems[d][j].vanished = true;
                } else {
                    gems[i][j].unknown = true;
                    gems[i][j].special = NONE;
                    gems[i][j].vanished = false;
                }
            }
        }
    }
}
