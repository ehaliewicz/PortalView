#include <genesis.h>
#include "common.h"
#include "draw.h"
#include "game.h"
#include "level.h"
#include "math3D.h"
#include "portal_map.h"
#include "portal_small_map.h"
#include "vertex.h"


#define PORTAL_QUEUE_SIZE 32

//static Vect2D_s32* cached_verts;
static u32* sector_visited_cache; // i don't know if this helps much, but we might be able to use it
static u8* sector_render_queue;

typedef struct {
    s16 x1, x2;
    s16 sector;
} render_sector_item;

static render_sector_item portal_queue[PORTAL_QUEUE_SIZE];
static u8 queue_read;
static u8 queue_write;

static u8 mask(u8 idx) { return idx & (PORTAL_QUEUE_SIZE-1); }

static void queue_push(render_sector_item i) { 
    portal_queue[mask(queue_write++)] = i;
}

static render_sector_item queue_pop() {
    return portal_queue[mask(queue_read++)];
}

static int queue_size()     { return queue_write - queue_read; }
static int queue_empty()    { return queue_size() == 0 ; }
static int queue_full()     { return queue_size() == PORTAL_QUEUE_SIZE; }

void init_portal_renderer() {
    //vert_transform_cache = MEM_alloc(sizeof(u32) * cur_portal_map->num_verts);
    queue_read = 0;
    queue_write = 0;

    sector_visited_cache = MEM_alloc(sizeof(u32) * cur_portal_map->num_sectors);   
}

void clear_portal_cache() {
    memset(sector_visited_cache, 0, sizeof(u32) * cur_portal_map->num_sectors);
}

void portal_rend(u16 src_sector, u32 cur_frame) {
    static const portal_map* map = &portal_level_1;


    render_sector_item queue_item = { .x1 = 0, .x2 = W-1, .sector = src_sector };

    queue_push(queue_item);

    int visited = 0;
    int skipped_backfacing_walls = 0;

    while(!queue_empty()) {
        visited++;
        render_sector_item cur_item = queue_pop();

        s16 sector = cur_item.sector;

        // if this sector has been visited 32 times, or is already being currently rendered, skip it
        if(sector_visited_cache[sector] >= 1) { //} & 0b101) { //0x21) {
             continue; // Odd = still rendering, 0x20 = give up
        }

        sector_visited_cache[sector]++;

        u16 window_min = cur_item.x1;
        u16 window_max = cur_item.x2;

        s16 wall_offset = sector_wall_offset(sector, map);
        s16 portal_offset = sector_portal_offset(sector, map);
        s16 num_walls = sector_num_walls(sector, map);

        s16 floor_height = sector_floor_height(sector, map); 
        s16 ceil_height = sector_ceil_height(sector, map);
        
        
        u16 init_v1_idx = map->walls[wall_offset];
        vertex init_v1 = map->vertexes[init_v1_idx];
        Vect2D_s32 prev_transformed_vert = transform_map_vert(init_v1.x, init_v1.y);
        
        s16 last_frontfacing_wall = -1;
        
        u16 prev_v2_idx; // = map->walls[wall_offset];

        for(s16 i = 0; i < num_walls; i++) {
            //u16 v1_idx = map->walls[wall_offset+i];
	        u16 v2_idx = map->walls[wall_offset+i+1];
            //vis_range rng = map->wall_vis_ranges[portal_offset+i];
            u16 ang = cur_player_pos.ang;
            
            ang = 1024-ang;
            ang &= 1023;
            
            u16 norm_angle = cur_portal_map->wall_norm_angles[portal_offset+i];
            s16 diff = abs(norm_angle - ang) % ANGLE_360_DEGREES;
            //diff &= 1023;
            if (diff > ANGLE_180_DEGREES) { diff = ANGLE_360_DEGREES - diff; } 
            s16 backfacing = diff < 180;


            //u8 frontfacing = (rng.angles[0] <= ang && rng.angles[1] >= ang);
            //if(!frontfacing && rng.two_ranges) {
            //    frontfacing = (rng.angles[2] <= ang && rng.angles[3] >= ang);
            //}
            u16 wall_idx = wall_offset+i;

            if(wall_idx == 1) {
                char buf[32];
                sprintf(buf, "cur wall norm ang: %lu  ", norm_angle);
                BMP_drawText(buf, 1, 7);
                sprintf(buf, "diff %lu  ", diff);
                BMP_drawText(buf, 1, 8);
                sprintf(buf, "backfacing: %lu  ", backfacing);
                BMP_drawText(buf, 1, 9);
            }

            
            //if(backfacing) { 
            //    skipped_backfacing_walls++;
            //    prev_v2_idx = v2_idx;
            //    continue;
            //}

	        vertex v2 = map->vertexes[v2_idx];
	    
            s16 portal_sector = map->portals[portal_offset+i];
            int is_portal = portal_sector != -1;

            

            volatile Vect2D_s32 trans_v1;
            if (last_frontfacing_wall == i-1) {
                // if we didn't backface cull the previous wall, reuse the result of it's transformed v2 as our v1
                trans_v1 = prev_transformed_vert;
            } else {
                // else grab the vertex and transform it
                vertex v1 = map->vertexes[prev_v2_idx];
                trans_v1 = transform_map_vert(v1.x, v1.y);
            }
            // = (prev_vert == i-1) ? prev_transformed_vert : transform_map_vert(v1.x, v1.y);

            volatile Vect2D_s32 trans_v2 = transform_map_vert(v2.x, v2.y);
            last_frontfacing_wall = i;
            prev_transformed_vert = trans_v2;


            vis_query_result vis = is_visible(trans_v1, trans_v2, window_min, window_max);
            if(vis.visible) {
                if (is_portal) {

                    // TODO: draw step up from floor
                    // draw step down from ceiling
                    s16 neighbor_floor_height = sector_floor_height(portal_sector, cur_portal_map);
                    s16 neighbor_ceil_height = sector_ceil_height(portal_sector, cur_portal_map);
                    if(neighbor_floor_height > floor_height && neighbor_floor_height < ceil_height) {
                        draw_wall_from_verts(trans_v1, trans_v2, neighbor_floor_height, floor_height, window_min, window_max);
                    }
                    if(neighbor_ceil_height < ceil_height && neighbor_ceil_height > floor_height) {
                        draw_wall_from_verts(trans_v1, trans_v2, neighbor_ceil_height, ceil_height, window_min, window_max);
                    }

                    if (neighbor_ceil_height > floor_height && neighbor_floor_height < ceil_height && !queue_full()) {

                        queue_item.x1 = vis.x1;
                        queue_item.x2 = vis.x2;
                        queue_item.sector = portal_sector;
                        queue_push(queue_item);
                    }
                } else {
                    draw_wall_from_verts(trans_v1, trans_v2, ceil_height, floor_height, window_min, window_max);
                }
            }
        }
        sector_visited_cache[sector]++;
    }

    char buf[32];
    sprintf(buf, "sectors visited: %i", visited);
    BMP_drawText(buf, 1, 3);
    sprintf(buf, "backfacing walls/portals: %i", skipped_backfacing_walls);
    BMP_drawText(buf, 1, 4);


}
