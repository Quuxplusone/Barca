#pragma once

#include "Board.h"

struct opponent_is_thinking {};

Board process_image(unsigned char (*im)[3], int w, int h);
