
#include <assert.h>
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
    int pop;
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
    for (int i=0; i < (int)pixels.size(); ++i) {
        ci += pixels[i].i;
        cj += pixels[i].j;
    }
    ci /= pop;
    cj /= pop;
    assert(ci >= 0 && cj >= 0);
    cx = ci;
    cy = cj;
    if (pop == 47*47) {
        is_square = true;
    }
}

static bool has_colored_circle(unsigned char (*im)[3], int w, int h,
                        int x, int y, unsigned int color_)
{
    unsigned char color[3] = { color_>>16, color_>>8, color_ };
    for (int j=y; j < y+47; ++j) {
        for (int i=x; i < x+47; ++i) {
            if (memcmp(im[j*w+i], color, 3) == 0) {
                return true;
            }
        }
    }
    return false;
}

static int square_weight(unsigned char (*im)[3], int w, int h,
                         int x, int y)
{
    unsigned char dark[3] = { 0x66, 0x33, 0x00 };
    unsigned char light[3] = { 0xFF, 0xFF, 0xCC };
    unsigned char blue[3] = { 0x27, 0x27, 0x8F };
    unsigned char white[3] = { 0xFF, 0xFF, 0xFF };
    int darks = 0;
    int lights = 0;
    for (int j=y; j < y+47; ++j) {
        for (int i=x; i < x+16; ++i) {
            assert(0 < j && j < h);
            assert(0 < i && i < w);
            darks += (memcmp(im[j*w+i], dark, 3) == 0);
            lights += (memcmp(im[j*w+i], light, 3) == 0);
        }
    }
    return 16*(darks/4) + (lights/4);
}

Board process_image(unsigned char (*im)[3], int w, int h)
{
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
            if (uf.pop[pn] < 47*45) {
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
    if (squares.size() < 60) { 
        printf("The screenshot doesn't have enough squares! (%d<100-12-4)\n",
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
    assert(board.right == board.left + 9*47);
    assert(board.bottom == board.top + 9*47);

    /* Identify the positions of all the pieces. */
    int mine_found = 0;
    int yours_found = 0;
    for (int x=0; x < 10; ++x) {
        for (int y=0; y < 10; ++y) {
            const int sx = board.left+47*x - 47/2;
            const int sy = board.top+47*y - 47/2;
            Player dummy_att;
            Player *att = &dummy_att;
            if (has_colored_circle(im,w,h, sx,sy, 0x00CC00)) {
                att = &board.attacker;
            } else if (has_colored_circle(im,w,h, sx,sy, 0xFF3300)) {
                /* If we're not expected to make a move, just bail and sleep. */
                throw "it's the Flash AI's turn";
            }
            switch (int weight = square_weight(im,w,h, sx,sy)) {
                case 178: board.my_pieces[mine_found++] = Piece(LION, x,y); *att = ME; break;
                case 296: board.my_pieces[mine_found++] = Piece(ELEPHANT, x,y); *att = ME; break;
                case 143: board.my_pieces[mine_found++] = Piece(MOUSE, x,y); *att = ME; break;
                case 1063: board.my_pieces[yours_found++] = Piece(LION, x,y); *att = YOU; break;
                case 911: board.my_pieces[yours_found++] = Piece(ELEPHANT, x,y); *att = YOU; break;
                case 1268: board.my_pieces[yours_found++] = Piece(MOUSE, x,y); *att = YOU; break;
                case 0: /* empty square */ break;
                default: printf("Unrecognized weight %d, might be a mouse cursor\n", weight); break;
            }
        }
    }
    if (mine_found != 6) {
        throw "Found fewer than 6 of my (white) pieces";
    } else if (yours_found != 6) {
        throw "Found fewer than 6 of your (black) pieces";
    } else if (board.attacker != ME && board.attacker != YOU) {
        /* This happens when pieces are in transit. Of course the odds are that
         * we'll already have failed to find squares or pieces by now, but just
         * in case, we'll bail here. */
        throw "Neither side seems to be the attacker";
    }

    return board;
}
