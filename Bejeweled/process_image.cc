
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>
#include "ImageFmtc.h"
#include "SimplePng.h"
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

    int center_x, center_y;

    PixelRegion() { }
    PixelRegion(int pop_): pop(pop_) { }

    void push_back(int i, int j) {
        pixels.push_back(Pixel(i,j));
    }
    void analyze(unsigned char (*im)[3], int w, int h);
    bool includes(int x, int y) {
        double dx = abs(x - center_x);
        double dy = abs(y - center_y);
        return (dx*dx + dy*dy < pop/2);
    }

    Gem to_gem() const;

    void draw(unsigned char (*im)[3], int w, int h) const;
};

void PixelRegion::analyze(unsigned char (*im)[3], int w, int h)
{
    assert((int)pixels.size() == pop);
    double ci = 0.0;
    double cj = 0.0;
    double cr = 0.0, cg = 0.0, cb = 0.0;
    for (int i=0; i < (int)pixels.size(); ++i) {
        Pixel& p = pixels[i];
        ci += p.i;
        cj += p.j;
        cr += im[p.j*w+p.i][0];
        cg += im[p.j*w+p.i][1];
        cb += im[p.j*w+p.i][2];
    }
    ci /= pop;
    cj /= pop;
    assert(ci >= 0 && cj >= 0);
    this->center_x = ci;
    this->center_y = cj;
    this->color[0] = cr / pop;
    this->color[1] = cg / pop;
    this->color[2] = cb / pop;
}

Gem PixelRegion::to_gem() const
{
    Gem gem;
    struct {
        int rgb;
        int color;
        SpecialGem special;
    } seen_in_the_wild[] = {
        { 0xef0832, RED, NONE },
        { 0xef2044, RED, NONE },
        { 0xef1f42, RED, NONE },
        { 0xee304f, RED, NONE },
        { 0xee3453, RED, NONE },
        { 0xef1038, RED, NONE },
        { 0xe1415a, RED, NONE }, // +5
        { 0xe45d72, RED, NONE }, // +5
        { 0xf58e43, ORANGE, NONE },
        { 0xf68936, ORANGE, NONE },
        { 0xf9b051, ORANGE, NONE },
        { 0xf49e5e, ORANGE, NONE },
        { 0xe7cdbe, ORANGE, NONE },
        { 0xf89a4a, ORANGE, NONE },
        { 0xf28d53, ORANGE, NONE }, // +5
        { 0xf1742d, ORANGE, NONE }, // +5
        { 0xf2d736, YELLOW, NONE },
        { 0xf9f046, YELLOW, NONE },
        { 0xf3da49, YELLOW, NONE },
        { 0xf1da65, YELLOW, NONE },
        { 0xf3d97c, YELLOW, NONE }, // +5
        { 0x15e14c, GREEN, NONE },
        { 0x1de153, GREEN, NONE },
        { 0x14dd49, GREEN, NONE },
        { 0x30e358, GREEN, NONE },
        { 0x26e158, GREEN, NONE },
        { 0x45e270, GREEN, NONE },
        { 0x75eb90, GREEN, NONE }, // +5
        { 0x2e96f6, BLUE, NONE },
        { 0x349bf6, BLUE, NONE },
        { 0x3aa1f6, BLUE, NONE },
        { 0x35a0ed, BLUE, NONE },
        { 0x58b9f6, BLUE, NONE }, // +5
        { 0xf220ef, PURPLE, NONE },
        { 0xf32df0, PURPLE, NONE },
        { 0xf347f0, PURPLE, NONE },
        { 0xf137ed, PURPLE, NONE },
        { 0xf159ee, PURPLE, NONE },
        { 0xf322de, PURPLE, NONE },
        { 0xea79e7, PURPLE, NONE }, // +5
        { 0xf1f1f1, WHITE, NONE },
        { 0xf7ddf6, WHITE, NONE },

        { 0xf13436, RED, FLAME_GEM },
        { 0xef2331, RED, FLAME_GEM },
        { 0xee1232, RED, FLAME_GEM },
        { 0xed3442, RED, FLAME_GEM },
        { 0xf2c538, YELLOW, FLAME_GEM },
        { 0xf2b42b, YELLOW, FLAME_GEM },
        { 0xf2bb33, YELLOW, FLAME_GEM },
        { 0x43d94c, GREEN, FLAME_GEM },
        { 0x77b836, GREEN, FLAME_GEM },
        { 0x57d147, GREEN, FLAME_GEM },
        { 0x2edd46, GREEN, FLAME_GEM },
        { 0x78c747, GREEN, FLAME_GEM },
        { 0x5cc844, GREEN, FLAME_GEM },
        { 0x65c93e, GREEN, FLAME_GEM },
        { 0x63d14b, GREEN, FLAME_GEM },
        { 0x20dd45, GREEN, FLAME_GEM },
        { 0x8ac441, GREEN, FLAME_GEM },
        { 0x92c04a, GREEN, FLAME_GEM },
        { 0x80c33f, GREEN, FLAME_GEM },
        { 0x94c33a, GREEN, FLAME_GEM },
        { 0x85bb37, GREEN, FLAME_GEM },
        { 0xa0b839, GREEN, FLAME_GEM },
        { 0x97b939, GREEN, FLAME_GEM },
        { 0xa7bc41, GREEN, FLAME_GEM },
        { 0x6fc241, GREEN, FLAME_GEM },
        { 0x88ae33, GREEN, FLAME_GEM },
        { 0x5ac836, GREEN, FLAME_GEM },
        { 0x7fb62e, GREEN, FLAME_GEM },
        { 0x4dcb42, GREEN, FLAME_GEM },
        { 0x6cbf36, GREEN, FLAME_GEM },
        { 0x48cb46, GREEN, FLAME_GEM },
        { 0xbab239, GREEN, FLAME_GEM },
        { 0x5898cc, BLUE, FLAME_GEM },
        { 0x4b9de1, BLUE, FLAME_GEM },
        { 0x33a0f7, BLUE, FLAME_GEM },
        { 0x8d948f, BLUE, FLAME_GEM },
        { 0x6ca1be, BLUE, FLAME_GEM },
        { 0xc69a5b, BLUE, FLAME_GEM },
        { 0xbe905b, BLUE, FLAME_GEM },
        { 0xe39935, BLUE, FLAME_GEM },
        { 0xe69733, BLUE, FLAME_GEM },
        { 0x869d9b, BLUE, FLAME_GEM },
        { 0x54a2d4, BLUE, FLAME_GEM },
        { 0x86a3a7, BLUE, FLAME_GEM },
        { 0x83a0a9, BLUE, FLAME_GEM },
        { 0x3daff8, BLUE, FLAME_GEM },
        { 0x48a3ea, BLUE, FLAME_GEM },
        { 0x8facaf, BLUE, FLAME_GEM },
        { 0x5faad8, BLUE, FLAME_GEM },
        { 0xc9a66d, BLUE, FLAME_GEM },
        { 0xb39a6b, BLUE, FLAME_GEM },
        { 0x98a18d, BLUE, FLAME_GEM },
        { 0xa3a180, BLUE, FLAME_GEM },
        { 0x7a90a3, BLUE, FLAME_GEM },
        { 0x91a0a0, BLUE, FLAME_GEM },
        { 0x3691e9, BLUE, FLAME_GEM },
        { 0xef47a3, PURPLE, FLAME_GEM },
        { 0xe77b44, PURPLE, FLAME_GEM },
        { 0xed5683, PURPLE, FLAME_GEM },
        { 0xef648d, PURPLE, FLAME_GEM },
        { 0xf16a95, PURPLE, FLAME_GEM },
        { 0xef657e, PURPLE, FLAME_GEM },
        { 0xf350b4, PURPLE, FLAME_GEM },
        { 0xef7d89, PURPLE, FLAME_GEM },
        { 0xef5d97, PURPLE, FLAME_GEM },
        { 0xf457bc, PURPLE, FLAME_GEM },
        { 0xea8b4e, PURPLE, FLAME_GEM },
        { 0xf04d99, PURPLE, FLAME_GEM },
        { 0xed6073, PURPLE, FLAME_GEM },
        { 0xf136c7, PURPLE, FLAME_GEM },
        { 0xee6d6a, PURPLE, FLAME_GEM },
        { 0xf0d0a3, WHITE, FLAME_GEM },
        { 0xf0e1c6, WHITE, FLAME_GEM },
        { 0xf1d1b0, WHITE, FLAME_GEM },
        { 0xedca9a, WHITE, FLAME_GEM },
        { 0xefd5ba, WHITE, FLAME_GEM },
        { 0xeec58b, WHITE, FLAME_GEM },
        { 0xeeb871, WHITE, FLAME_GEM },
        { 0xf1e9dd, WHITE, FLAME_GEM },
        { 0xeee1d1, WHITE, FLAME_GEM },

        { 0xf39aa7, RED, STAR_GEM },
        { 0xf28c9b, RED, STAR_GEM },
        { 0xf7c5a6, ORANGE, STAR_GEM },
        { 0xf6bd97, ORANGE, STAR_GEM },
        { 0xf3c09e, ORANGE, STAR_GEM },
        { 0xf8edba, YELLOW, STAR_GEM },
        { 0xf6e9a6, YELLOW, STAR_GEM },
        { 0xeae2b4, YELLOW, STAR_GEM },
        { 0xdde7ee, BLUE, STAR_GEM },
        { 0xf0a1ee, PURPLE, STAR_GEM },
        { 0xeca5ea, PURPLE, STAR_GEM },
        { 0xe9b8ed, PURPLE, STAR_GEM },
        { 0xf095ed, PURPLE, STAR_GEM },

        { 0xfa9af6, 0, HYPERCUBE },
        { 0xf4dea7, 0, HYPERCUBE },
        { 0xf9adf3, 0, HYPERCUBE },
        { 0xf6f692, 0, HYPERCUBE },
        { 0xf78ef4, 0, HYPERCUBE },
        { 0xf4d794, 0, HYPERCUBE },
    };
    gem.color = -1;
    double min_dist3 = 4*255*255;
    for (size_t i=0; i < sizeof seen_in_the_wild / sizeof *seen_in_the_wild; ++i) {
        double dr = abs(color[0] - ((seen_in_the_wild[i].rgb >> 16) & 0xff));
        double dg = abs(color[1] - ((seen_in_the_wild[i].rgb >> 8) & 0xff));
        double db = abs(color[2] - ((seen_in_the_wild[i].rgb >> 0) & 0xff));
        double dist3 = (dr*dr + dg*dg + db*db);
        if (dist3 < min_dist3) {
            min_dist3 = dist3;
            gem.color = seen_in_the_wild[i].color;
            gem.special = seen_in_the_wild[i].special;
        }
    }
    printf("(%d,%d) with color 0x%02x%02x%02x -- %s (%.2g)\n", center_x, center_y, color[0], color[1], color[2],
           gem.str().c_str(), min_dist3);
    if (min_dist3 > 200) throw "unexpectedly large color difference";
    gem.vanished = false;
    gem.unknown = false;
    return gem;
}

void PixelRegion::draw(unsigned char (*im)[3], int w, int h) const
{
    for (int i=0; i < (int)pixels.size(); ++i) {
        const Pixel& p = pixels[i];
        memcpy(im[p.j*w+p.i], color, 3);
    }
}

static void bresenham_circle(unsigned char (*im)[3], int w, int h,
                             int cx, int cy, int radius,
                             unsigned char color[3])
{
    int f = 1-radius;
    int ddF_x = 1;
    int ddF_y = -2*radius;
    int x = 0;
    int y = radius;
#define SETPIXEL(i,j) if ((j) >= 0 && (i) >= 0 && (j) <= h && (i) <= w) memcpy(im[(j)*w+(i)], color, 3)
    SETPIXEL(cx, cy + radius);
    SETPIXEL(cx, cy - radius);
    SETPIXEL(cx + radius, cy);
    SETPIXEL(cx - radius, cy);
    while (x < y) {
        if (f >= 0) {
            --y;
            ddF_y += 2;
            f += ddF_y;
        }
        ++x;
        ddF_x += 2;
        f += ddF_x;
        SETPIXEL(cx + x, cy + y);
        SETPIXEL(cx - x, cy + y);
        SETPIXEL(cx + x, cy - y);
        SETPIXEL(cx - x, cy - y);
        SETPIXEL(cx + y, cy + x);
        SETPIXEL(cx - y, cy + x);
        SETPIXEL(cx + y, cy - x);
        SETPIXEL(cx - y, cy - x);
    }
#undef SETPIXEL
}

bool is_bright(unsigned char px[3])
{
    const int r = px[0], g = px[1], b = px[2];
    return (r > 200) || (g > 170 && r < 100) || (b > 210 && r < 100);
}

Board process_image(unsigned char (*im)[3], int w, int h)
{
    /* Black out the "non-gem" parts of the image. */
    for (int j=0; j < h; ++j) {
        for (int i=0; i < w; ++i) {
            if (!is_bright(im[j*w+i])) {
                memset(im[j*w+i], '\0', 3);
            }
        }
    }
    WritePNG("/tmp/blacked.png", im, w, h);

    /* Find connected bright regions. */
    UnionFind uf(w*h);
    for (int j=0; j < h-1; ++j) {
        for (int i=0; i < w-1; ++i) {
            if (is_bright(im[j*w+i]) && is_bright(im[j*w+(i+1)]))
              uf.merge(j*w+i, j*w+(i+1));
            if (is_bright(im[j*w+i]) && is_bright(im[(j+1)*w+i]))
              uf.merge(j*w+i, (j+1)*w+i);
        }
    }
    std::vector<PixelRegion> regions;
    std::map<int, size_t> regionmap;
    for (int j=0; j < h; ++j) {
        for (int i=0; i < w; ++i) {
            int n = j*w+i;
            int pn = uf.findparent(n);
            assert(pn <= n);
            if (uf.pop[pn] < 700 || uf.pop[pn] > 2600) {
                /* do nothing with this region */
            } else {
                if (pn == n) {
                    regionmap[pn] = regions.size();
                    regions.push_back(PixelRegion(uf.pop[pn]));
                }
                regions[regionmap[pn]].push_back(i,j);
            }
        }
    }
    regionmap.clear();
    printf("number of gem-looking regions: %d\n", (int)regions.size());

    for (int i=0; i < (int)regions.size(); ++i) {
        PixelRegion &pr = regions[i];
        pr.analyze(im, w, h);
        pr.draw(im, w, h);
    }

    /* We expect to see at least 6 gems in each row (allowing for the fact that we
     * have a hard time spotting the rotating hypercubes, and there might be a couple
     * of those in any given row). */
    std::map<int,int> most_popular_rows;
    std::map<int,int> most_popular_cols;
    for (int i=0; i < (int)regions.size(); ++i) {
        int col = regions[i].center_x / 14;
        int row = regions[i].center_y / 14;
        most_popular_rows[row] += 1;
        most_popular_rows[row-1] += 1;
        most_popular_rows[row+1] += 1;
        most_popular_cols[col] += 1;
        most_popular_cols[col-1] += 1;
        most_popular_cols[col+1] += 1;
    }
    std::vector<PixelRegion*> gems;
    for (int i=0; i < (int)regions.size(); ++i) {
        int col = regions[i].center_x / 14;
        int row = regions[i].center_y / 14;
        if (most_popular_rows[row] >= 6 && most_popular_cols[col] >= 6) {
            gems.push_back(&regions[i]);
        }
    }

    for (int i=0; i < (int)gems.size(); ++i) {
        /* Highlight the located gems. */
        const PixelRegion &pr = *gems[i];
        unsigned char color[3] = {255,255,255};
        bresenham_circle(im, w, h, pr.center_x, pr.center_y, sqrt(pr.pop)-2, color);
    }

    if (gems.size() < 10) {
        printf("Found %d < 10 gem-looking objects! Try again.\n", (int)gems.size());
        WritePNG("/tmp/real-gems.png", im, w, h);
        throw "try again";
    }

    /* Now try to fill in the gaps: find gem-looking regions within a small radius
     * of the gaps in our grid. */

    Board board;
    const int N = 8;
    board.N = N;
    board.gems.resize(N);
    for (int i=0; i < N; ++i) {
        board.gems[i].resize(N);
    }
    board.top = gems.front()->center_y;
    board.left = gems.front()->center_x;
    board.bottom = gems.back()->center_y;
    board.right = gems.back()->center_x;
    for (int i=0; i < (int)gems.size(); ++i) {
        board.top = std::min(board.top, gems[i]->center_y);
        board.left = std::min(board.left, gems[i]->center_x);
        board.bottom = std::max(board.bottom, gems[i]->center_y);
        board.right = std::max(board.right, gems[i]->center_x);
    }
    int inserted_hypercubes = 0;
    for (int j=0; j < N; ++j) {
        printf("Row %d:\n", j);
        for (int i=0; i < N; ++i) {
            const int expected_cx = board.left + i * (board.right - board.left) / (N-1);
            const int expected_cy = board.top + j * (board.bottom - board.top) / (N-1);
            bool found_one = false;
            unsigned char green[3] = {0,255,0};
            bresenham_circle(im, w, h, expected_cx, expected_cy, 5, green);
            for (int k = 0; k < (int)regions.size(); ++k) {
                if (regions[k].includes(expected_cx, expected_cy)) {
                    board.gems[j][i] = regions[k].to_gem();
                    found_one = true;
                    break;
                }
            }
            if (!found_one) {
                board.gems[j][i] = Gem::hypercube();
                printf("inserted a hypercube\n");
                inserted_hypercubes += 1;
            }
        }
    }

    WritePNG("/tmp/real-gems.png", im, w, h);

    if (inserted_hypercubes > 9) {
        printf("Too many hypercubes! The board looks like this:\n");
        board.print();
        throw "too many hypercubes";
    }

    return board;
}
