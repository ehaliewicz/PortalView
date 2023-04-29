#include <genesis.h>
#include "colors.h"
#include "portal_map.h"
#include "sector_group.h"
#include "vertex.h"

static const s16 sectors[29*SECTOR_SIZE] = {
    0, 0, 6, 0,
    7, 6, 7, 1,
    15, 13, 6, 2,
    22, 19, 7, 3,
    30, 26, 4, 4,
    35, 30, 8, 5,
    44, 38, 5, 6,
    50, 43, 6, 7,
    57, 49, 0, 8,
    57, 49, 6, 9,
    64, 55, 4, 10,
    69, 59, 9, 11,
    79, 68, 6, 12,
    86, 74, 9, 13,
    96, 83, 5, 14,
    102, 88, 4, 15,
    107, 92, 4, 16,
    112, 96, 4, 17,
    117, 100, 4, 18,
    122, 104, 4, 19,
    127, 108, 4, 20,
    132, 112, 4, 21,
    137, 116, 0, 22,
    137, 116, 4, 23,
    142, 120, 0, 24,
    142, 120, 4, 25,
    147, 124, 6, 26,
    154, 130, 4, 27,
    159, 134, 4, 28,
};

static const s16 sector_group_params[29*NUM_SECTOR_GROUP_PARAMS] = {
0,0,0,0,0<<4,150<<4,8,7,
0,0,0,0,0<<4,150<<4,8,7,
0,0,0,0,0<<4,150<<4,8,7,
0,0,0,0,0<<4,150<<4,8,7,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,-20<<4,150<<4,10,7,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,-20<<4,160<<4,5,4,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,40<<4,150<<4,5,4,
0,0,0,0,40<<4,150<<4,5,4,
0,0,0,0,40<<4,150<<4,5,4,
0,0,0,0,40<<4,150<<4,5,4,
0,0,0,0,0<<4,120<<4,5,11,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,0<<4,150<<4,5,4,
0,0,0,0,30<<4,100<<4,10,4,
0,0,0,0,30<<4,100<<4,10,4,
};

static const s16 sector_group_triggers[29*8] = {
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
};

static const u8 sector_group_types[29] = {
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
NO_TYPE,
};

static const u16 walls[167] = {
    51, 20, 17, 15, 12, 50, 51, 
    51, 57, 48, 0, 75, 58, 20, 51, 
    48, 49, 5, 3, 2, 0, 48, 
    5, 49, 50, 12, 59, 76, 60, 5, 
    17, 16, 14, 15, 17, 
    14, 16, 21, 22, 33, 32, 67, 69, 14, 
    22, 23, 24, 30, 33, 22, 
    21, 18, 25, 24, 23, 22, 21, 
// sector 8 is empty
    25, 26, 54, 29, 30, 24, 25, 
    26, 9, 27, 54, 26, 
    9, 7, 6, 73, 10, 43, 40, 28, 27, 9, 
    30, 29, 28, 40, 41, 31, 30, 
    62, 63, 47, 44, 42, 43, 10, 77, 78, 62, 
    31, 46, 64, 67, 32, 31, 
    33, 38, 36, 32, 33, 
    33, 30, 56, 38, 33, 
    30, 31, 55, 56, 30, 
    31, 32, 36, 55, 31, 
    56, 55, 36, 38, 56, 
    31, 41, 45, 46, 31, 
    41, 42, 44, 45, 41, 
// sector 22 is empty
    46, 47, 65, 64, 46, 
// sector 24 is empty
    70, 69, 67, 66, 70, 
    70, 66, 65, 47, 63, 13, 70, 
    25, 58, 75, 26, 25, 
    77, 60, 76, 78, 77, 
};
static const s16 portals[138] ={
    1, -1, 4, -1, 3, -1, 
    -1, -1, 2, -1, 27, -1, 0, 
    -1, 3, -1, -1, -1, 1, 
    2, -1, 0, -1, -1, 28, -1, 
    -1, 5, -1, 0, 
    4, -1, -1, 6, 15, 14, -1, -1, 
    7, -1, 9, 16, 5, 
    -1, -1, -1, -1, 6, -1, 
    
    27, 10, -1, 12, 6, -1, 
    -1, 11, -1, 9, 
    -1, -1, -1, -1, 13, -1, 12, -1, 10, 
    9, -1, 11, -1, 20, 17, 
    -1, 26, -1, 21, -1, 11, -1, 28, -1, 
    20, 23, -1, 5, 18, 
    16, 19, 18, 5, 
    6, 17, 19, 15, 
    12, 18, 19, 16, 
    14, 15, 19, 17, 
    17, 18, 15, 16, 
    12, 21, -1, 14, 
    -1, 13, -1, 20, 
    
    -1, 26, -1, 14, 
    
    -1, -1, -1, 26, 
    25, -1, 23, 13, -1, -1, 
    -1, 1, -1, 9, 
    -1, 3, -1, 13, 
};

static const u8 wall_normal_quadrants[138] ={
    QUADRANT_3,
    FACING_DOWN,
    FACING_DOWN,
    FACING_DOWN,
    QUADRANT_2,
    FACING_UP,
    FACING_RIGHT,
    FACING_RIGHT,
    QUADRANT_2,
    FACING_LEFT,
    FACING_LEFT,
    FACING_LEFT,
    QUADRANT_1,
    FACING_DOWN,
    QUADRANT_1,
    FACING_UP,
    FACING_UP,
    FACING_UP,
    QUADRANT_0,
    QUADRANT_3,
    FACING_LEFT,
    QUADRANT_0,
    FACING_RIGHT,
    FACING_RIGHT,
    FACING_RIGHT,
    FACING_RIGHT,
    FACING_RIGHT,
    FACING_DOWN,
    FACING_LEFT,
    FACING_UP,
    FACING_UP,
    FACING_UP,
    FACING_RIGHT,
    QUADRANT_3,
    FACING_DOWN,
    QUADRANT_2,
    FACING_LEFT,
    FACING_UP,
    FACING_RIGHT,
    FACING_RIGHT,
    FACING_DOWN,
    FACING_LEFT,
    QUADRANT_1,
    FACING_UP,
    FACING_RIGHT,
    FACING_DOWN,
    FACING_LEFT,
    FACING_LEFT,
    FACING_LEFT,
    FACING_RIGHT,
    FACING_DOWN,
    FACING_DOWN,
    FACING_LEFT,
    FACING_UP,
    FACING_UP,
    FACING_RIGHT,
    QUADRANT_2,
    FACING_LEFT,
    FACING_UP,
    FACING_DOWN,
    FACING_DOWN,
    FACING_DOWN,
    FACING_DOWN,
    QUADRANT_1,
    FACING_UP,
    FACING_UP,
    FACING_UP,
    QUADRANT_0,
    FACING_RIGHT,
    FACING_RIGHT,
    FACING_DOWN,
    FACING_LEFT,
    FACING_LEFT,
    FACING_UP,
    FACING_LEFT,
    FACING_UP,
    FACING_RIGHT,
    FACING_RIGHT,
    FACING_RIGHT,
    QUADRANT_3,
    FACING_LEFT,
    FACING_LEFT,
    FACING_LEFT,
    FACING_DOWN,
    QUADRANT_2,
    FACING_LEFT,
    QUADRANT_0,
    FACING_RIGHT,
    QUADRANT_3,
    FACING_DOWN,
    QUADRANT_2,
    FACING_UP,
    FACING_RIGHT,
    QUADRANT_2,
    FACING_LEFT,
    QUADRANT_1,
    FACING_DOWN,
    QUADRANT_1,
    FACING_UP,
    QUADRANT_0,
    FACING_LEFT,
    QUADRANT_0,
    FACING_RIGHT,
    QUADRANT_3,
    FACING_DOWN,
    FACING_LEFT,
    FACING_UP,
    FACING_RIGHT,
    FACING_RIGHT,
    QUADRANT_2,
    FACING_LEFT,
    FACING_UP,
    FACING_DOWN,
    FACING_LEFT,
    FACING_UP,
    QUADRANT_0,
    FACING_DOWN,
    QUADRANT_1,
    FACING_UP,
    QUADRANT_0,
    FACING_UP,
    FACING_RIGHT,
    FACING_DOWN,
    FACING_LEFT,
    FACING_RIGHT,
    FACING_RIGHT,
    QUADRANT_3,
    FACING_DOWN,
    FACING_LEFT,
    FACING_UP,
    FACING_UP,
    FACING_RIGHT,
    FACING_DOWN,
    FACING_LEFT,
    FACING_DOWN,
    FACING_LEFT,
    FACING_UP,
    FACING_RIGHT,
};

static const u8 wall_colors[138*4] ={
1, 1, 1, 1,
4, 1, 1, 0,
0, 1, 1, 1,
4, 1, 1, 0,
3, 1, 1, 1,
0, 1, 1, 7,
0, 1, 1, 7,
0, 1, 1, 7,
0, 1, 1, 1,
4, 1, 1, 0,
2, 9, 9, 1,
4, 1, 1, 0,
0, 1, 1, 1,
0, 1, 1, 7,
0, 1, 1, 1,
4, 1, 1, 0,
4, 1, 1, 0,
4, 1, 1, 0,
0, 1, 1, 1,
0, 1, 1, 1,
0, 1, 1, 7,
0, 1, 1, 1,
4, 1, 1, 0,
4, 1, 1, 0,
0, 9, 9, 1,
4, 1, 1, 0,
0, 1, 1, 1,
0, 1, 1, 1,
0, 1, 1, 1,
0, 1, 1, 1,
0, 1, 1, 1,
0, 1, 1, 9,
0, 1, 1, 9,
0, 1, 1, 1,
0, 1, 7, 1,
0, 1, 1, 1,
0, 1, 1, 8,
0, 1, 1, 9,
0, 1, 1, 1,
0, 1, 1, 9,
0, 1, 1, 1,
0, 1, 7, 1,
0, 1, 1, 1,
5, 1, 1, 0,
5, 1, 1, 0,
5, 1, 1, 0,
5, 1, 1, 0,
0, 1, 9, 1,
5, 1, 1, 0,
9, 9, 9, 9,
0, 1, 1, 1,
5, 1, 1, 0,
0, 1, 1, 1,
0, 1, 1, 1,
5, 1, 1, 0,
0, 1, 1, 9,
0, 1, 1, 1,
0, 1, 1, 9,
0, 1, 1, 1,
2, 1, 1, 0,
2, 1, 1, 0,
2, 1, 1, 0,
2, 1, 1, 0,
0, 7, 7, 1,
5, 1, 1, 0,
0, 7, 7, 1,
5, 1, 1, 0,
0, 7, 7, 1,
0, 1, 1, 1,
0, 1, 1, 9,
0, 1, 1, 1,
0, 1, 1, 9,
0, 1, 1, 1,
0, 1, 7, 1,
0, 1, 1, 9,
0, 1, 1, 1,
0, 1, 1, 9,
0, 1, 1, 1,
0, 1, 1, 9,
0, 1, 1, 1,
0, 1, 1, 9,
0, 9, 9, 1,
0, 1, 1, 9,
0, 1, 1, 1,
0, 1, 1, 1,
0, 1, 1, 8,
0, 1, 1, 1,
0, 1, 7, 1,
0, 1, 1, 1,
0, 7, 1, 1,
0, 1, 1, 1,
0, 1, 1, 1,
0, 1, 1, 1,
0, 1, 1, 1,
0, 7, 1, 1,
0, 1, 1, 1,
0, 1, 1, 1,
0, 1, 1, 1,
0, 7, 1, 1,
0, 1, 1, 1,
0, 1, 1, 1,
0, 1, 1, 1,
0, 7, 1, 1,
0, 1, 1, 1,
0, 1, 7, 1,
0, 1, 7, 1,
0, 1, 7, 1,
0, 1, 7, 1,
0, 1, 1, 1,
0, 1, 1, 1,
0, 1, 1, 9,
0, 1, 1, 1,
5, 1, 1, 0,
0, 1, 1, 1,
5, 1, 1, 0,
0, 1, 1, 1,
0, 1, 1, 9,
0, 1, 1, 1,
0, 1, 1, 8,
0, 1, 1, 1,
5, 1, 1, 0,
5, 1, 1, 0,
5, 1, 1, 0,
0, 1, 1, 1,
0, 1, 1, 1,
0, 1, 1, 8,
0, 1, 1, 1,
0, 1, 1, 1,
0, 1, 1, 9,
0, 1, 1, 9,
0, 1, 1, 10,
0, 1, 1, 1,
0, 1, 1, 10,
9, 1, 1, 1,
0, 1, 1, 10,
0, 1, 1, 1,
0, 1, 1, 10,
0, 1, 1, 1,
};

#define VERT(x1,y1) { .x = (x1 * 1.3), .y = ((-y1) * 1.3) } 
static const vertex vertexes[79] = {
    VERT(0,-40),
    VERT(1000,-40),
    VERT(180,-40),
    VERT(400,-40),
    VERT(1000,-20),
    VERT(600,-40),
    VERT(300,-20),
    VERT(160,-20),
    VERT(1000,20),
    VERT(20,-20),
    VERT(580,-20),
    VERT(1000,440),
    VERT(600,460),
    VERT(580,440),
    VERT(360,440),
    VERT(360,460),
    VERT(240,440),
    VERT(240,460),
    VERT(20,440),
    VERT(1000,460),
    VERT(0,460),
    VERT(160,440),
    VERT(160,380),
    VERT(160,320),
    VERT(160,240),
    VERT(20,240),
    VERT(20,160),
    VERT(70,90),
    VERT(220,90),
    VERT(220,160),
    VERT(220,240),
    VERT(380,240),
    VERT(380,340),
    VERT(220,340),
    VERT(260,240),
    VERT(350,240),
    VERT(350,320),
    VERT(350,340),
    VERT(260,320),
    VERT(260,340),
    VERT(380,90),
    VERT(380,150),
    VERT(530,150),
    VERT(530,90),
    VERT(530,200),
    VERT(430,200),
    VERT(430,240),
    VERT(530,240),
    VERT(-200,-200),
    VERT(800,-200),
    VERT(800,660),
    VERT(-200,660),
    VERT(1000,700),
    VERT(20,380),
    VERT(70,160),
    VERT(350,260),
    VERT(260,260),
    VERT(-200,230),
    VERT(0,240),
    VERT(600,280),
    VERT(600,15),
    VERT(1000,340),
    VERT(580,170),
    VERT(580,240),
    VERT(440,310),
    VERT(510,310),
    VERT(510,350),
    VERT(440,350),
    VERT(1000,500),
    VERT(440,440),
    VERT(510,440),
    VERT(430,150),
    VERT(1000,800),
    VERT(440,-20),
    VERT(1000,-50),
    VERT(0,160),
    VERT(600,70),
    VERT(580,15),
    VERT(580,70),
};
const portal_map building_test_map  = {
    .num_sectors = 29,
    .num_sector_groups = 29,
    .num_walls = 138,
    .num_verts = 79,
    .sectors = sectors,
    .sector_group_types = sector_group_types,
    .sector_group_params = sector_group_params,
    .sector_group_triggers = sector_group_triggers,
    .walls = walls,
    .portals = portals,
    .vertexes = vertexes,
    .wall_colors = wall_colors,
    .wall_norm_quadrants = wall_normal_quadrants,
    .has_pvs = 0,
    .name = "building test",
    .num_things = 0,
};