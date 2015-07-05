
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>
#include "ImageFmtc.h"
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

    bool is_circle;
    int center_x, center_y;
    double radius;

    PixelRegion() { }
    PixelRegion(int pop_, unsigned char *c): pop(pop_), is_circle(false) {
        memcpy(color, c, 3);
    }
    void push_back(int i, int j) {
        pixels.push_back(Pixel(i,j));
    }
    void analyze();

    static bool by_radius(const PixelRegion *a, const PixelRegion *b) {
        assert(a->is_circle && b->is_circle);
        return (a->radius < b->radius);
    }
    static bool by_xy(const PixelRegion *a, const PixelRegion *b) {
        assert(a->is_circle && b->is_circle);
        int dy = (int)(b->center_y - a->center_y) / 16;
        int dx = (int)(b->center_x - a->center_x) / 16;
        return (dy != 0) ? (dy > 0) : (dx > 0);
    }
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
    /* If the pixels are all roughly the same distance from the
     * center of the region, then it's a circle. */
    double avgdist = 0.0;
    for (int i=0; i < (int)pixels.size(); ++i) {
        double dx = pixels[i].i - ci;
        double dy = pixels[i].j - cj;
        double dist = sqrt(dx*dx + dy*dy);
        avgdist += dist;
    }
    avgdist /= pop;
    bool might_be_circle = true;
    for (int i=0; might_be_circle && i < (int)pixels.size(); ++i) {
        double dx = pixels[i].i - ci;
        double dy = pixels[i].j - cj;
        double dist = sqrt(dx*dx + dy*dy);
        double dd = fabs(dist - avgdist);
        if (dist < 20 || dd > 5.0)
          might_be_circle = false;
    }
    if (might_be_circle) {
        /* It's a big region where all the pixels are roughly the same
         * nonzero distance from the center; i.e., a circle. */
        this->is_circle = true;
        this->center_x = ci;
        this->center_y = cj;
        this->radius = avgdist;
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
#define SETPIXEL(i,j) memcpy(im[(j)*w+(i)], color, 3)
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
            if (uf.pop[pn] < 50) {
                /* do nothing with this tiny region */
            } else {
                if (pn == n)
                  regions[pn] = PixelRegion(uf.pop[pn], im[pn]);
                regions[pn].push_back(i,j);
            }
        }
    }
    printf("number of regions: %d\n", (int)regions.size());
    if (regions.size() < 104) {
        printf("The screenshot doesn't have enough connected regions! (%d<104)\n",
                (int)regions.size());
        printf("Try again.\n");
        throw "try again";
    }

    std::vector<PixelRegion*> circles;
    for (std::map<int,PixelRegion>::iterator it = regions.begin();
            it != regions.end(); ++it) {
        PixelRegion &pr = it->second;
        pr.analyze();
        if (pr.is_circle)
          circles.push_back(&pr);
    }
    printf("number of circles: %d\n", (int)circles.size());
    if (circles.size() < 49) { 
        printf("The screenshot doesn't have enough circles! (%d<49)\n",
                (int)circles.size());
        printf("Try again.\n");
        throw "try again";
    }
    /* Sort the circles by radius. */
    std::sort(circles.begin(), circles.end(), PixelRegion::by_radius);

    for (int i=0; i < (int)circles.size(); ++i) {
        /* Highlight the located circles. */
        const PixelRegion &pr = *circles[i];
        unsigned char color[3] = {255,0,255};
        assert(pr.is_circle);
        bresenham_circle(im, w, h, pr.center_x, pr.center_y, pr.radius, color);
    }
    WritePPM6("/tmp/circles.ppm", im, w, h);

    /* Identify 49 circles that have roughly the same radius. */
    int start_of_run = 0;
    for (int i=0; i < (int)circles.size()-1; ++i) {
        if (circles[i]->radius + 5 < circles[i+1]->radius) {
            if (i+1 - start_of_run == 49) {
                circles.resize(i+1);
                break;
            } else {
                start_of_run = i+1;
            }
        }
    }
    if (start_of_run != (int)circles.size() - 49) {
        printf("The screenshot doesn't have exactly 49 equal-sized circles!\n");
        printf("Try again.\n");
        throw "try again";
    }

    for (int i=0; i < 49; ++i)
      circles[i] = circles[i+start_of_run];
    circles.resize(49);

    /* Sort the 49 circles by x/y coordinates. */
    std::sort(circles.begin(), circles.end(), PixelRegion::by_xy);

    Board board;
    board.top = circles[0]->center_y;
    board.left = circles[0]->center_x;
    board.bottom = circles[48]->center_y;
    board.right = circles[48]->center_x;
    board.goals_left = 0;

    /* For each circle, decide which direction the arrow is facing,
     * and whether it is a colored goal circle. */
    for (int i=0; i < 49; ++i) {
        PixelRegion &pr = *circles[i];
        BoardCircle &bc = board.circles[i/7][i%7];
        int cx = pr.center_x;
        int cy = pr.center_y;
        unsigned char mycolor[3];
        memcpy(mycolor, im[cy*w+cx], 3);
        if (mycolor[0] < 240 || mycolor[1] < 240 || mycolor[2] < 240) {
            board.goals_left += 1;
            bc.is_goal_circle = true;
        } else {
            bc.is_goal_circle = false;
        }
        bc.vanished = false;
        bc.dir = NONE;
        bool nn = false, ss = false;
        bool ee = false, ww = false;
        for (int d = 1; d < (int)pr.radius-6; ++d) {
            if (memcmp(mycolor, im[(cy-d)*w+cx], 3) != 0)
              nn = true;
            if (memcmp(mycolor, im[(cy+d)*w+cx], 3) != 0)
              ss = true;
            if (memcmp(mycolor, im[cy*w+(cx+d)], 3) != 0)
              ee = true;
            if (memcmp(mycolor, im[cy*w+(cx-d)], 3) != 0)
              ww = true;
        }
        if (nn && (ee == ww) && !ss) {
            bc.dir = NORTH;
        } else if (ee && (nn == ss) && !ww) {
            bc.dir = EAST;
        } else if (ww && (nn == ss) && !ee) {
            bc.dir = WEST;
        } else if (ss && (ee == ww) && !nn) {
            bc.dir = SOUTH;
        } else if (!nn && !ss && !ee && !ww) {
            bc.dir = NONE;
        } else {
            printf("Circle %d=(%d,%d) at pixel (%d,%d) is unintelligible!\n",
                    i, i/7, i%7, pr.center_x, pr.center_y);
            printf("Try again.\n");
            throw "try again";
        }
    }
    return board;
}
