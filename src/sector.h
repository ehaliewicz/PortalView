#ifndef SECTOR_H
#define SECTOR_H

#include <genesis.h>
#include "portal_map.h"

#define NUM_SECTOR_TYPES 4

typedef enum {
    NO_TYPE = 0,
    FLASHING = 1,
    DOOR = 2,
    LIFT = 3,
} sector_type;

typedef enum {
    CLOSED,
    GOING_UP,
    OPEN,
    GOING_DOWN
} door_lift_state;


/*
typedef struct {
    s8 light;
    s16 orig_height;
    u8 ticks_left;
    u8 state; // 0 closed, 1 going up, 2 open, 3 going on
} sector_param;
*/

#define SECTOR_PARAM_LIGHT_IDX 0
#define SECTOR_PARAM_ORIG_HEIGHT_IDX 1
#define SECTOR_PARAM_STATE_IDX 2
#define SECTOR_PARAM_TICKS_LEFT_IDX 3

#define NUM_SECTOR_PARAMS 4
#define NUM_SECTOR_PARAMS_SHIFT 2
extern s16* live_sector_parameters;
//extern sector_param* live_sector_parameters;


s16 get_sector_orig_height(u16 sect_idx);
s8 get_sector_light_level(u16 sect_idx);
u16 get_sector_ticks_left(u16 sect_idx);
u16 get_sector_state(u16 sect_idx);

void run_sector_processes();

#endif