
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>
#include "Board.h"

struct UnionFind {
    int *nodes, *pop;
    int cap;
    UnionFind(int cap_);
    ~UnionFind() { delete [] nodes; delete [] pop; }
    void merge(int a, int b);
    int findparent(int a);
};

UnionFind::UnionFind(int cap_): cap(cap_)
{
    assert(cap_ > 0);
    nodes = new int[cap_];
    pop = new int[cap_];
    for (int i=0; i < cap_; ++i) {
        nodes[i] = i;
        pop[i] = 1;
    }
}

void UnionFind::merge(int a, int b)
{
    assert(0 <= a && a < b && b < cap);
    int pa = findparent(a);
    int pb = findparent(b);
    assert(0 <= pa && pa < cap);
    assert(0 <= pb && pb < cap);
    assert(pop[pa] >= 1);
    assert(pop[pb] >= 1);
    if (pa == pb) return;
    if (pa > pb) {
        int t = pa; pa = pb; pb = t;
    }
    nodes[pb] = pa;
    pop[pa] += pop[pb];
    pop[pb] = 0;
    assert(2 <= pop[pa] && pop[pa] <= cap);
}

int UnionFind::findparent(int a)
{
    assert(0 <= a && a < cap);
    while (a != nodes[a]) {
        a = nodes[a];
        assert(0 <= a && a < cap);
    }
    return a;
}

struct Pixel {
    int i, j;
    Pixel(int i_, int j_): i(i_), j(j_) { }
};

struct PixelRegion {
    size_t pop;
    unsigned char color[3];
    std::vector<Pixel> pixels;

    bool is_square;
    int cx, cy;

    PixelRegion() { }
    PixelRegion(int pop_, unsigned char *c): pop(pop_), is_square(false) {
        memcpy(color, c, 3);
    }
    void push_back(int i, int j) {
        pixels.push_back(Pixel(i,j));
    }
    void analyze();
};

void PixelRegion::analyze()
{
    assert(pixels.size() == pop);
    double ci = 0.0;
    double cj = 0.0;
    int xmin = INT_MAX, ymin = INT_MAX;
    int xmax = INT_MIN, ymax = INT_MIN;
    for (int i=0; i < (int)pixels.size(); ++i) {
        ci += pixels[i].i;
        cj += pixels[i].j;
        if (pixels[i].i < xmin) xmin = pixels[i].i;
        else if (pixels[i].i > xmax) xmax = pixels[i].i;
        if (pixels[i].j < ymin) ymin = pixels[i].j;
        else if (pixels[i].j > ymax) ymax = pixels[i].j;
    }
    ci /= pop;
    cj /= pop;
    assert(ci >= 0 && cj >= 0);
    cx = ci;
    cy = cj;
    if (xmax == xmin+46 && ymax == ymin+46 && pop > 40*20) {
        is_square = true;
        cx = (xmax + xmin) / 2;
        cy = (ymax + ymin) / 2;
    }
}

static bool has_colored_circle(unsigned char (*im)[3], int w, int h,
                        int x, int y, unsigned int color_)
{
    unsigned char color[3] = { color_>>16, color_>>8, color_ };
    int count = 0;
    for (int j=y; j < y+47; ++j) {
        for (int i=x; i < x+47; ++i) {
            count += (memcmp(im[j*w+i], color, 3) == 0);
        }
    }
    return (count > 50);
}

static int square_weight(unsigned char (*im)[3], int w, int h,
                         int x, int y)
{
    unsigned char dark[3] = { 0x66, 0x33, 0x00 };
    unsigned char light[3] = { 0xFF, 0xFF, 0xCC };
    unsigned char blue[3] = { 0x27, 0x27, 0x8F };
    unsigned char white[3] = { 0xFF, 0xFF, 0xFF };
    unsigned char crown_orange[3] = { 0xFF, 0x99, 0x00 };
    unsigned char crown_yellow[3] = { 0xFF, 0xCC, 0x00 };
    int darks = 0;
    int lights = 0;
    for (int j=y+18; j < y+39; ++j) {
        for (int i=x; i < x+20; ++i) {
            assert(0 < j && j < h);
            assert(0 < i && i < w);
            darks += (memcmp(im[j*w+i], dark, 3) == 0);
            lights += (memcmp(im[j*w+i], light, 3) == 0);
        }
    }
    return 16*(darks/4) + (lights/4);
}

static bool square_crowned(unsigned char (*im)[3], int w, int h,
                           int x, int y)
{
    unsigned char crown_orange[3] = { 0xFF, 0x99, 0x00 };
    unsigned char crown_yellow[3] = { 0xFF, 0xCC, 0x00 };
    int oranges = 0;
    for (int j=y; j < y+23; ++j) {
        for (int i=x; i < x+47; ++i) {
            assert(0 < j && j < h);
            assert(0 < i && i < w);
            oranges += (memcmp(im[j*w+i], crown_orange, 3) == 0);
        }
    }
    return (oranges > 100);  /* should be 160 */
}

static bool board_has_crowns(unsigned char (*im)[3], int w, int h,
                             Board &board)
{
    for (int x=3; x <= 6; x += 3) {
        for (int y=3; y <= 6; y += 3) {
            const int sx = board.left+47*x - 47/2;
            const int sy = board.top+47*y - 47/2;
            
            int weight = square_weight(im,w,h, sx,sy);
            if (weight == 0) continue;

            if (square_crowned(im,w,h, sx,sy)) {
                board.attacker = (weight < 500) ? BLACK : WHITE;
                return true;
            }
        }
    }
    return false;
}


Board process_image(unsigned char (*im)[3], int w, int h, bool &found_red_circle)
{
    found_red_circle = false;

    /* Find connected regions of the same color; then discard
     * any regions of area less than 50. */
    UnionFind uf(w*h);
    for (int j=0; j < h-1; ++j) {
        for (int i=0; i < w-1; ++i) {
            if (memcmp(im[j*w+i], im[j*w+(i+1)], 3) == 0)
              uf.merge(j*w+i, j*w+(i+1));
            if (memcmp(im[j*w+i], im[(j+1)*w+i], 3) == 0)
              uf.merge(j*w+i, (j+1)*w+i);
        }
    }
    std::map<int, PixelRegion> regions;
    for (int j=0; j < h; ++j) {
        for (int i=0; i < w; ++i) {
            int n = j*w+i;
            int pn = uf.findparent(n);
            if (uf.pop[pn] < 47*20) {
                /* do nothing with this tiny region */
            } else {
                if (pn == n)
                  regions[pn] = PixelRegion(uf.pop[pn], im[pn]);
                regions[pn].push_back(i,j);
            }
        }
    }
    printf("number of regions: %d\n", (int)regions.size());

    std::vector<PixelRegion*> squares;
    for (std::map<int,PixelRegion>::iterator it = regions.begin();
            it != regions.end(); ++it) {
        PixelRegion &pr = it->second;
        pr.analyze();
        if (pr.is_square)
          squares.push_back(&pr);
    }
    printf("number of squares: %d\n", (int)squares.size());
    if (squares.size() < 50) { 
        printf("The screenshot doesn't have enough squares! (%d<50)\n",
                (int)squares.size());
        printf("Try again.\n");
        throw "try again";
    }

    Board board;
    board.left = squares[0]->cx;
    board.right = squares[0]->cx;
    board.top = squares[0]->cy;
    board.bottom = squares[0]->cy;

    for (int i=0; i < (int)squares.size(); ++i) {
        /* Identify the corners of the board. */
        if (squares[i]->cx < board.left) board.left = squares[i]->cx;
        else if (squares[i]->cx > board.right) board.right = squares[i]->cx;
        if (squares[i]->cy < board.top) board.top = squares[i]->cy;
        else if (squares[i]->cy > board.bottom) board.bottom = squares[i]->cy;
    }
    if ((board.right != board.left + 9*47) ||
        (board.bottom != board.top + 9*47))
    {
        printf("board.left = %d, board.right = %d\n", board.left, board.right);
        printf("board.top = %d, board.bottom = %d\n", board.top, board.bottom);
        throw "board was the wrong size";
    }
    
    /* Identify the positions of all the pieces. */
    int white_found = 0;
    int black_found = 0;
    bool found_attacker = false;
    for (int x=0; x < 10; ++x) {
        for (int y=0; y < 10; ++y) {
            const int sx = board.left+47*x - 47/2;
            const int sy = board.top+47*y - 47/2;
            Player dummy_att;
            Player *att = &dummy_att;
            if (has_colored_circle(im,w,h, sx,sy, 0x00CC00)) {
                printf("Found green circle at square %d,%d\n", x,y);
                att = &board.attacker;
                found_attacker = true;
            } else if (has_colored_circle(im,w,h, sx,sy, 0xFF3300)) {
                printf("Found red circle at square %d,%d\n", x,y);
                att = &board.attacker;
                found_attacker = true;
                found_red_circle = true;  /* We're not expecting to move;
                                           * the Flash AI is thinking. */
            } else if (has_colored_circle(im,w,h, sx,sy, 0x40FF40)) {
                printf("Found bright green circle at square %d,%d\n", x,y);
                att = &board.attacker;
                found_attacker = true;
                found_red_circle = true;  /* We're not expecting to move; the
                                           * human is in the middle of a move. */
            }
            switch (int weight = square_weight(im,w,h, sx,sy)) {
                case 236 ... 240: board.white_pieces[white_found++] = Piece(LION, x,y); *att = WHITE; break;
                case 356 ... 375: board.white_pieces[white_found++] = Piece(ELEPHANT, x,y); *att = WHITE; break;
                case 215 ... 235: board.white_pieces[white_found++] = Piece(MOUSE, x,y); *att = WHITE; break;
                case 780: board.black_pieces[black_found++] = Piece(LION, x,y); *att = BLACK; break;
                case 644 ... 645: board.black_pieces[black_found++] = Piece(ELEPHANT, x,y); *att = BLACK; break;
                case 953: board.black_pieces[black_found++] = Piece(MOUSE, x,y); *att = BLACK; break;
                case 0: /* empty square */ break;
                default: printf("Unrecognized weight %d at (%d,%d), might be a mouse cursor\n", weight, x,y); break;
            }
        }
    }
    if (white_found != 6) {
        throw "Found fewer than 6 white pieces";
    } else if (black_found != 6) {
        throw "Found fewer than 6 black pieces";
    }
    board.update_scaredness();
    if (!found_attacker) {
        /* This happens when pieces are in transit, or at the end of the game. */
        if (board_has_crowns(im,w,h, board)) {
            /* One side or the other was crowned. */
        } else {
            throw "Neither side seems to be the attacker yet";
        }
    }

    return board;
}
