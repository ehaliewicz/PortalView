#include <genesis.h>
#include "colors.h"
#include "portal_map.h"
#include "slope.h"
#include "vertex.h"


// wall offset, portal offset, number of walls, floor height, ceiling height, floor color, ceiling color, slope_start_wall_idx, slope_end_wall_idx
static const s16 sectors[153] = {
    0, 0, 6, 60<<4, 200<<4, BLUE_IDX, BLUE_IDX,   // floor normally 100 here
    7, 6, 5, 100<<4, 200<<4, BLUE_IDX, BLUE_IDX,  
    13, 11, 4, 60<<4, 200<<4, BLUE_IDX, BLUE_IDX,   // floor normally 100 here
    18, 15, 4, 120<<4, 240<<4, BLUE_IDX, LIGHT_BLUE_IDX,
    23, 19, 4, 140<<4, 280<<4, BLUE_IDX, BLUE_IDX,
    28, 23, 4, 160<<4, 320<<4, BLUE_IDX, LIGHT_BLUE_IDX,
    33, 27, 4, 180<<4, 350<<4, BLUE_IDX, BLUE_IDX,
    38, 31, 5, 200<<4, 350<<4, BLUE_IDX, LIGHT_BLUE_IDX,
    44, 36, 4, 100<<4, 200<<4, BLUE_IDX, BLUE_IDX,
    49, 40, 4, 100<<4, 200<<4, BLUE_IDX, BLUE_IDX,
    54, 44, 4, 200<<4, 350<<4, BLUE_IDX, BLUE_IDX,
    59, 48, 4, 80<<4, 180<<4, RED_IDX, LIGHT_RED_IDX,
    64, 52, 4, 60<<4, 160<<4, BLUE_IDX, BLUE_IDX,
    69, 56, 4, 40<<4, 140<<4, RED_IDX, LIGHT_RED_IDX,
    74, 60, 4, 20<<4, 120<<4, BLUE_IDX, BLUE_IDX,
    79, 64, 5, 0<<4, 100<<4, RED_IDX, LIGHT_RED_IDX,
    85, 69, 4, 0<<4, 100<<4, RED_IDX, RED_IDX,
};

static const u8 wall_floor_slope_types[90] = {
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    SLOPE_TRANSITION_RIGHT, SLOPE_PORTAL, SLOPE_TRANSITION_LEFT, NO_SLOPE, 
    SLOPE_TRANSITION_RIGHT, SLOPE_PORTAL, SLOPE_TRANSITION_LEFT, NO_SLOPE, 
    SLOPE_TRANSITION_RIGHT, SLOPE_PORTAL, SLOPE_TRANSITION_LEFT, NO_SLOPE, 
    SLOPE_TRANSITION_RIGHT, SLOPE_PORTAL, SLOPE_TRANSITION_LEFT, NO_SLOPE, 
    SLOPE_TRANSITION_RIGHT, SLOPE_PORTAL, SLOPE_TRANSITION_LEFT, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
};

static const u8 wall_ceil_slope_types[90] = {
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE,
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE,
    SLOPE_TRANSITION_LEFT, NO_SLOPE, SLOPE_TRANSITION_RIGHT, SLOPE_PORTAL,
    SLOPE_TRANSITION_LEFT, NO_SLOPE, SLOPE_TRANSITION_RIGHT, SLOPE_PORTAL, 
    SLOPE_TRANSITION_LEFT, NO_SLOPE, SLOPE_TRANSITION_RIGHT, SLOPE_PORTAL,
    SLOPE_TRANSITION_LEFT, NO_SLOPE, SLOPE_TRANSITION_RIGHT, SLOPE_PORTAL,
    SLOPE_TRANSITION_LEFT, NO_SLOPE, SLOPE_TRANSITION_RIGHT, SLOPE_PORTAL,
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
    NO_SLOPE, NO_SLOPE, NO_SLOPE, NO_SLOPE, 
};

static const s16 floor_slopes[17*2] = {
    -1,-1, 
    -1,-1,
    3,1,
    3,1,
    3,1,
    3,1,
    3,1,
    -1,-1,
    -1,-1,
    -1,-1,
    -1,-1,
    -1,-1,
    -1,-1,
    -1,-1,
    -1,-1,
    -1,-1,
    -1,-1,
};

static const s16 ceil_slopes[17*2] = {
    -1,-1, 
    -1,-1,
    -1,-1,
    1,3,
    1,3,
    1,3,
    1,3,
    -1,-1,
    -1,-1,
    -1,-1,
    -1,-1,
    -1,-1,
    -1,-1,
    -1,-1,
    -1,-1,
    -1,-1,
    -1,-1,
};


static const u16 walls[90] = {
    0, 1, 2, 3, 4, 5, 0, 
    2, 6, 7, 21, 3, 2, 
    4, 8, 9, 5, 4, 
    8, 10, 11, 9, 8, 
    10, 12, 13, 11, 10, 
    12, 14, 15, 13, 12, 
    14, 16, 17, 15, 14, 
    16, 18, 19, 20, 17, 16, 
    21, 22, 23, 3, 21, 
    22, 24, 25, 23, 22, 
    18, 26, 27, 19, 18, 
    6, 28, 29, 7, 6, 
    28, 30, 31, 29, 28, 
    30, 32, 33, 31, 30, 
    32, 34, 35, 33, 32, 
    34, 36, 37, 38, 35, 34, 
    36, 39, 40, 37, 36, 
};
static const s16 portals[73] = {
    -1, -1, 1, -1, 2, -1, 
    -1, 11, -1, 8, 0, 
    -1, 3, -1, 0, 
    -1, 4, -1, 2, 
    -1, 5, -1, 3, 
    -1, 6, -1, 4, 
    -1, 7, -1, 5, 
    -1, 10, -1, -1, 6, 
    -1, 9, -1, 1, 
    -1, -1, -1, 8, 
    -1, -1, -1, 7, 
    -1, 12, -1, 1, 
    -1, 13, -1, 11, 
    -1, 14, -1, 12, 
    -1, 15, -1, 13, 
    -1, 16, -1, -1, 14, 
    -1, -1, -1, 15, 
};

static const u8 wall_normal_quadrants[73] ={
    FACING_DOWN,
    FACING_LEFT,
    FACING_LEFT,
    FACING_UP,
    FACING_UP,
    FACING_RIGHT,
    FACING_DOWN,
    FACING_LEFT,
    FACING_UP,
    FACING_UP,
    FACING_RIGHT,
    FACING_LEFT,
    FACING_UP,
    FACING_RIGHT,
    FACING_DOWN,
    QUADRANT_2,
    QUADRANT_1,
    QUADRANT_0,
    FACING_DOWN,
    QUADRANT_2,
    QUADRANT_1,
    QUADRANT_0,
    QUADRANT_3,
    FACING_DOWN,
    FACING_LEFT,
    QUADRANT_0,
    QUADRANT_3,
    FACING_DOWN,
    FACING_LEFT,
    FACING_UP,
    FACING_RIGHT,
    QUADRANT_3,
    FACING_LEFT,
    FACING_UP,
    FACING_RIGHT,
    FACING_RIGHT,
    FACING_LEFT,
    FACING_UP,
    FACING_RIGHT,
    FACING_DOWN,
    QUADRANT_2,
    FACING_UP,
    QUADRANT_3,
    FACING_DOWN,
    QUADRANT_3,
    FACING_LEFT,
    QUADRANT_0,
    FACING_RIGHT,
    QUADRANT_2,
    QUADRANT_1,
    QUADRANT_0,
    FACING_RIGHT,
    QUADRANT_2,
    QUADRANT_1,
    QUADRANT_0,
    QUADRANT_3,
    QUADRANT_1,
    QUADRANT_0,
    QUADRANT_3,
    QUADRANT_3,
    QUADRANT_1,
    QUADRANT_0,
    QUADRANT_3,
    QUADRANT_2,
    QUADRANT_1,
    QUADRANT_0,
    QUADRANT_3,
    QUADRANT_2,
    QUADRANT_2,
    FACING_LEFT,
    QUADRANT_0,
    QUADRANT_3,
    QUADRANT_2,
};

static const wall_col wall_colors[73] ={
    {.mid_col = GREEN_IDX},
    {.mid_col = GREEN_IDX},
    {.upper_col = LIGHT_GREEN_IDX, .lower_col = GREEN_IDX},
    {.mid_col = LIGHT_GREEN_IDX},
    {.upper_col = LIGHT_GREEN_IDX, .lower_col = LIGHT_GREEN_IDX},
    {.mid_col = GREEN_IDX},
    // sector 1 walls
    {.mid_col = GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
    {.mid_col = LIGHT_GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
    // sector 2 walls
    {.mid_col = GREEN_IDX},
    {.upper_col = LIGHT_GREEN_IDX, .lower_col = LIGHT_GREEN_IDX},
    {.mid_col = GREEN_IDX},
    {.upper_col = LIGHT_GREEN_IDX, .lower_col = LIGHT_GREEN_IDX},
    // sector 3 walls
    {.mid_col = GREEN_IDX},
    {.upper_col = LIGHT_GREEN_IDX, .lower_col = LIGHT_GREEN_IDX},
    {.mid_col = GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
    // sector 4 walls
    {.mid_col = GREEN_IDX},
    {.upper_col = LIGHT_GREEN_IDX, .lower_col = LIGHT_GREEN_IDX},
    {.mid_col = LIGHT_GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
    // sector 5 walls
    {.mid_col =GREEN_IDX},
    {.upper_col = LIGHT_GREEN_IDX, .lower_col = LIGHT_GREEN_IDX},
    {.mid_col = LIGHT_GREEN_IDX},
    {.upper_col =GREEN_IDX, .lower_col =GREEN_IDX},
    // sector 6 walls
    {.mid_col =GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
    {.mid_col = LIGHT_GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
    // sector 7 walls
    {.mid_col =GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
    {.mid_col = LIGHT_GREEN_IDX},
    {.mid_col = GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
    // sector 8 walls
    {.mid_col = GREEN_IDX},
    {.upper_col = LIGHT_GREEN_IDX, .lower_col = LIGHT_GREEN_IDX},
    {.mid_col = GREEN_IDX},
    {.upper_col =GREEN_IDX, .lower_col =GREEN_IDX},
    // sector 9 walls
    {.mid_col = GREEN_IDX},
    {.mid_col = LIGHT_GREEN_IDX},
    {.mid_col = GREEN_IDX},
    {.upper_col =GREEN_IDX, .lower_col =GREEN_IDX},
    // sector 10 walls
    {.mid_col = GREEN_IDX},
    {.mid_col = GREEN_IDX},
    {.mid_col = GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
    // sector 11 walls
    {.mid_col = GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
    {.mid_col = LIGHT_GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
    // sector 12 walls
    {.mid_col = GREEN_IDX},
    {.upper_col = LIGHT_GREEN_IDX, .lower_col = LIGHT_GREEN_IDX},
    {.mid_col = GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col =GREEN_IDX},
    // sector 13 walls
    {.mid_col = GREEN_IDX},
    {.upper_col = LIGHT_GREEN_IDX, .lower_col = LIGHT_GREEN_IDX},
    {.mid_col = GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col =GREEN_IDX},
    // sector 14 walls
    {.mid_col = LIGHT_GREEN_IDX},
    {.upper_col = LIGHT_GREEN_IDX, .lower_col = LIGHT_GREEN_IDX},
    {.mid_col = GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
    // sector 15 walls
    {.mid_col = GREEN_IDX},
    {.upper_col = LIGHT_GREEN_IDX, .lower_col = LIGHT_GREEN_IDX},
    {.mid_col = GREEN_IDX},
    {.mid_col = GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
    // sector 16
    {.mid_col = GREEN_IDX},
    {.mid_col = LIGHT_GREEN_IDX},
    {.mid_col = GREEN_IDX},
    {.upper_col = GREEN_IDX, .lower_col = GREEN_IDX},
};

// #define VERT(x1,y1) { .x = (x1 * 6), .y = ((-y1) * 6) } 

#define VERT(x1,y1) { .x = (x1 * 2), .y = ((-y1) * 2) }
static const vertex vertexes[42] = {
    VERT(70,60),
    VERT(170,60),
    VERT(170,110),
    VERT(170,160),
    VERT(120,160),
    VERT(70,160),
    VERT(310,110),
    VERT(310,160),
    VERT(120,270),
    VERT(70,270),
    VERT(140,300),
    VERT(80,320),
    VERT(160,320),
    VERT(110,350),
    VERT(170,320),
    VERT(170,360),
    VERT(190,320),
    VERT(190,360),
    VERT(260,310),
    VERT(260,390),
    VERT(190,390),
    VERT(220,160),
    VERT(220,450),
    VERT(170,450),
    VERT(320,570),
    VERT(60,570),
    VERT(360,250),
    VERT(360,470),
    VERT(360,130),
    VERT(330,170),
    VERT(380,180),
    VERT(340,190),
    VERT(370,220),
    VERT(330,210),
    VERT(350,250),
    VERT(310,230),
    VERT(320,300),
    VERT(240,260),
    VERT(270,210),
    VERT(320,400),
    VERT(140,320),
    VERT(0,0),
};


// sector 9,10,16
sector_param sector_params[17] = {
    {.light = 0},
    {.light = 0},
    {.light = 0},
    {.light = -1},
    {.light = -1},
    {.light = -1},
    {.light = -1},
    {.light = -1},
    {.light = -2},
    {.light = -2}, // flashing
    {.light = -2}, // flashing
    {.light = -1},
    {.light = -1},
    {.light = -1},
    {.light = -1},
    {.light = -2},
    {.light = -2}, // flashing
};

sector_type sector_types[17] = {
    NO_TYPE,
    NO_TYPE,
    NO_TYPE,
    NO_TYPE,
    NO_TYPE,
    NO_TYPE,
    NO_TYPE,
    NO_TYPE,
    NO_TYPE,
    NO_TYPE, //FLASHING,
    NO_TYPE, //FLASHING,
    NO_TYPE,
    NO_TYPE,
    NO_TYPE,
    NO_TYPE,
    NO_TYPE,
    FLASHING
};

static const u8 pvs[32*4] = {
    // sector 0 pvs
    0b00111110,0b00011001,0b00000000,0b00000000,
    // sector 1 pvs
    0b00000000,0b00000000,0b00000000,0b00000000,
    // sector 2 pvs
    0b11111011,0b00011001,0b00000000,0b00000000,
    // sector 3 pvs
    0b11111101,0b00000000,0b00000000,0b00000000,
    // sector 4 pvs
    0b00000000,0b00000000,0b00000000,0b00000000,
    // sector 5 pvs
    0b00000000,0b00000000,0b00000000,0b00000000,
    // sector 6 pvs
    0b00000000,0b00000000,0b00000000,0b00000000,
    // sector 7 pvs
    0b00000000,0b00000000,0b00000000,0b00000000,
    // sector 8 pvs
    0b00000000,0b00000000,0b00000000,0b00000000,
    // sector 9 pvs
    0b00000000,0b00000000,0b00000000,0b00000000,
    // sector 10 pvs
    0b00000000,0b00000000,0b00000000,0b00000000,
    // sector 11 pvs
    0b00000000,0b00000000,0b00000000,0b00000000,
    // sector 12 pvs
    0b00000000,0b00000000,0b00000000,0b00000000,
    // sector 13 pvs
    0b00000000,0b00000000,0b00000000,0b00000000,
    // sector 14 pvs
    0b00000000,0b00000000,0b00000000,0b00000000,
    // sector 15 pvs
    0b00000000,0b00000000,0b00000000,0b00000000,
    // sector 16 pvs
    0b00000000,0b00000000,0b00000000,0b00000000,
};


portal_map overlapping_map = {
    .num_sectors = 17,
    .num_walls = 73,
    .num_verts = 42,
    .sectors = sectors,
    .sector_types = sector_types,
    .sector_params = sector_params,
    .walls = walls,
    .portals = portals,
    .vertexes = vertexes,
    .wall_colors = wall_colors,
    .wall_norm_quadrants = wall_normal_quadrants,
    .floor_slopes = floor_slopes,
    .wall_floor_slope_types = wall_floor_slope_types,
    .ceil_slopes = ceil_slopes,
    .wall_ceil_slope_types = wall_ceil_slope_types,
    .pvs = pvs
};
