#pragma once

#include <string>
#include <vector>

struct Move {
    int i, j;
    bool swap_downward;  // or else swap rightward
    int gems_vanished;
    int flame_gems, star_gems, hypercubes;
    int how_many_refills;
    int moves_visible;
    bool illegal;

    Move():
        i(0), j(0), swap_downward(false),
        gems_vanished(0),
        flame_gems(0), star_gems(0), hypercubes(0),
        how_many_refills(0), moves_visible(0),
        illegal(true)
    { }

    Move(int i, int j, bool swap_downward) :
        i(i), j(j), swap_downward(swap_downward),
        gems_vanished(0),
        flame_gems(0), star_gems(0), hypercubes(0),
        how_many_refills(0), moves_visible(0),
        illegal(false)
    { }

    int ti() const { return swap_downward ? i+1 : i; }
    int tj() const { return swap_downward ? j : j+1; }

    int fitness() const;
};

enum GemColor {
    RED,
    ORANGE,
    YELLOW,
    GREEN,
    BLUE,
    PURPLE,
    WHITE
};

enum SpecialGem {
    NONE,
    FLAME_GEM,  // 4 gems in a row
    STAR_GEM,   // 3 gems in each direction
    HYPERCUBE   // 5 gems in a row
};

struct Gem {
    int color;
    SpecialGem special;
    bool vanished;
    bool unknown;
    int length_of_vertical_match;
    int length_of_horizontal_match;

    static Gem hypercube() {
        Gem gem;
        gem.color = 0;
        gem.special = HYPERCUBE;
        gem.vanished = false;
        gem.unknown = false;
        return gem;
    }

    std::string str() const {
        const char *color_names[7] = {
            "red", "orange", "yellow", "green", "blue", "purple", "white"
        };
        switch (special) {
            case NONE: return color_names[color];
            case FLAME_GEM: return std::string("flame ") + color_names[color];
            case STAR_GEM: return std::string("star ") + color_names[color];
            case HYPERCUBE: return "hypercube";
        }
    }
};

struct Board {
    int N;
    std::vector<std::vector<Gem> > gems;
    int top, bottom, left, right;

    void blow_up(int i, int j);
    void refill();

    Move check_move(int i, int j, bool swap_downward);
    bool contains_matches();
    Move eval_move(int i, int j, bool swap_downward);

    std::vector<Move> find_all_moves() const;
    Move find_best_move() const;

    void print() const
    {
        for (int i=0; i < N; ++i) {
            for (int j=0; j < N; ++j) {
                const Gem &g = gems[i][j];
                char emph = (g.special == FLAME_GEM ? '!' : g.special == STAR_GEM ? '+' : ' ');
                char middle = (g.special == HYPERCUBE ? '%' : "ROYGBPW"[g.color]);
                if (g.unknown) {
                    printf(" ? ");
                } else if (g.vanished) {
                    printf(" _ ");
                } else {
                    printf("%c%c%c", emph, middle, emph);
                }
            }
            printf("\n");
        }
    }
};
