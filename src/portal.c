#include <genesis.h>
#include <kdebug.h>
#include "clip_buf.h"
#include "colors.h"
#include "common.h"
#include "config.h"
#include "debug.h"
#include "div_lut.h"
#include "ball.h"
#include "draw.h"
#include "game.h"
#include "level.h"
#include "math3D.h"
#include "portal_map.h"
#include "sector.h"
#include "textures.h"
#include "vertex.h"


#define PORTAL_QUEUE_SIZE 32

//static Vect2D_s32* cached_verts;
static u32* sector_visited_cache; // i don't know if this helps much, but we might be able to use it

// num_walls, sector_idx

// left_x, right_x, left_z_fix, right_z_fix, left_u, right_u, portal_sector_idx, EMPTY



void init_portal_renderer() {
    //vert_transform_cache = MEM_alloc(sizeof(u32) * cur_portal_map->num_verts);
    sector_visited_cache = MEM_alloc(sizeof(u32) * cur_portal_map->num_sectors);
}


void clear_portal_cache() {
    memset(sector_visited_cache, 0, sizeof(u32) * cur_portal_map->num_sectors);
}

void cleanup_portal_renderer() {
    MEM_free(sector_visited_cache);
}


// #define DEBUG_PORTAL_CLIP


#define MAX_DEPTH 32
int nwalls;

int post_project_backfacing_walls;
int walls_frustum_culled;
int pre_transform_backfacing_walls;
int portals_frustum_culled;
// u8* src_pvs

int sectors_scanned = 0;
void visit_graph(u16 src_sector, u16 sector, u16 x1, u16 x2, u32 cur_frame, uint8_t depth) {
    if(depth >= MAX_DEPTH) {
        return;
    }
    portal_map* map = (portal_map*)cur_portal_map;

    // if this sector has been visited 32 times, or is already being currently rendered, skip it
    if(sector_visited_cache[sector] & 0b1) { //} & 0b101) { //0x21) {
            return; // Odd = still rendering, 0x20 = give up
    }

    sector_visited_cache[sector]++; // = cur_frame;

    u16 window_min = x1; //cur_item.x1;
    u16 window_max = x2; //cur_item.x2;

    s16 wall_offset = sector_wall_offset(sector, map);
    s16 portal_offset = sector_portal_offset(sector, map);
    s16 num_walls = sector_num_walls(sector, map);




    s8 light_level = get_sector_light_level(sector);

    u16 init_v1_idx = map->walls[wall_offset];
    vertex init_v1 = map->vertexes[init_v1_idx];
    Vect2D_s16 prev_transformed_vert = transform_map_vert_16(init_v1.x, init_v1.y);

    s16 last_frontfacing_wall = -1;

    u16 prev_v2_idx = init_v1_idx;

    s16 intPx = fix32ToInt(cur_player_pos.x);
    s16 intPy = fix32ToInt(cur_player_pos.y);

    //int render_red_ball = (sector == 10);
    //object* objects_in_sect = objects_in_sector(sector);
    //int needs_object_clip_buffer = 0; //(objects_in_sect != NULL);
    //int needs_object_clip_buffer = render_red_ball;

    //clip_buf* obj_clip_buf;
    //if(needs_object_clip_buffer) {
    //    //KLog_U1("allocating object clip buffer in sector: ", sector);
    //    obj_clip_buf = alloc_clip_buffer();
    //    if(obj_clip_buf == NULL) {
    //        die("no more clip bufs");
    //    }
    //    copy_2d_buffer(window_min, window_max, obj_clip_buf);
    //}


    s16 floor_height = get_sector_floor_height(sector);
    s16 ceil_height = get_sector_ceil_height(sector);
    s16 rel_floor_height = (floor_height>>4) - (cur_player_pos.z>>(FIX32_FRAC_BITS));
    s16 rel_ceil_height = (ceil_height>>4) - (cur_player_pos.z>>(FIX32_FRAC_BITS));

    u8 floor_color = get_sector_floor_color(sector);
    u8 ceil_color = get_sector_ceil_color(sector);

    u16 sect_flags = sector_flags(sector, map);

    u8 cur_sect_type = cur_portal_map->sector_types[sector];


    light_params ceil_params, floor_params;
    cache_ceil_light_params(rel_ceil_height, ceil_color, light_level, &ceil_params);
    cache_floor_light_params(rel_floor_height, floor_color, light_level, &floor_params);

    for(s16 i = 0; i < num_walls; i++) {


        //const int debug = (sector == 0) && (i == 0);
        // this is plus 1 because we store the number of real walls,
        // but duplicate the first at the end of the wall list per sector, so we don't need modulo math
        s16 wall_idx = wall_offset+i+1;

        u16 portal_idx = portal_offset+i;
        s16 portal_sector = map->portals[portal_idx];
        int is_portal = portal_sector != -1;


        u16 v2_idx = map->walls[wall_idx];

        u16 old_prev_v2_idx = prev_v2_idx;
        prev_v2_idx = v2_idx;
        //if(is_portal) {
        //    u8 byte_idx = (portal_sector>>3);
        //    u8 bit_idx = (portal_sector&0b111);
        //    u8 in_pvs = src_pvs[byte_idx] & (1 << bit_idx);
        //    if(!in_pvs) {
        //        continue;
        //    }
        //}


        vertex v1 = map->vertexes[old_prev_v2_idx];
        vertex v2 = map->vertexes[v2_idx];
        u32 wall_len = getApproximatedDistance(v2.x - v1.x, v2.y - v1.y);

        volatile Vect2D_s16 trans_v1;

        if(last_frontfacing_wall == i-1) {
            trans_v1 = prev_transformed_vert;
        } else {
            if(0) { //if(debug) {
                KLog("VERT 1");
            }
            trans_v1 = transform_map_vert_16(v1.x, v1.y);
        }

        prev_v2_idx = v2_idx;


        normal_quadrant normal_dir = map->wall_norm_quadrants[portal_idx];

        u8 backfacing = 0;

        switch(normal_dir) {
            case QUADRANT_0:
                backfacing = (intPx < v2.x && intPy < v1.y);
                break;
            case QUADRANT_1:
                backfacing = (intPx > v1.x && intPy < v2.y);
                break;
            case QUADRANT_2:
                backfacing = (intPx > v2.x && intPy > v1.y);
                break;
            case QUADRANT_3:
                backfacing = (intPx < v1.x && intPy > v2.y);
                break;

            case FACING_UP:
                backfacing = (intPy < v1.y);
                break;
            case FACING_DOWN:
                backfacing = (intPy > v1.y);
                break;
            case FACING_LEFT:
                backfacing = (intPx > v1.x);
                break;
            case FACING_RIGHT:
                backfacing = (intPx < v1.x);
                break;
        }

        if(backfacing) {
            #ifdef DEBUG_PORTAL_CLIP
            pre_transform_backfacing_walls++;
            if(is_portal) {
                KLog_S1("pre-transform portal cull: ", portal_idx);
            }
            #endif
            continue;
        } else {
        }




        last_frontfacing_wall = i;
        if(0) { //if(debug) {
            KLog("VERT 2");
        }
        volatile Vect2D_s16 trans_v2 = transform_map_vert_16(v2.x, v2.y);
        prev_transformed_vert = trans_v2;

        texmap_params tmap_info = {};
        clip_result clipped = clip_map_vertex_16(&trans_v1, &trans_v2, &tmap_info, wall_len);

        u16 z_recip_v1 = z_recip_table_16[trans_v1.y>>TRANS_Z_FRAC_BITS];
        u16 z_recip_v2 = z_recip_table_16[trans_v2.y>>TRANS_Z_FRAC_BITS];
        s16 neighbor_floor_height;
        s16 neighbor_ceil_height;

        if(clipped == OFFSCREEN) {
            if(sector == src_sector && is_portal) {
                #ifdef DEBUG_PORTAL_CLIP
                if(is_portal) {
                    portals_frustum_culled++;
                    KLog_S1("portal fully near clipped: ", portal_idx);
                    KLog_S2("z1: ", trans_v1.y, " z2: ", trans_v2.y);
                }
                #endif
                if(trans_v1.y == 0) { trans_v1.y = (1<<2)<<TRANS_Z_FRAC_BITS; z_recip_v1 = 65535; }
                if(trans_v2.y == 0) { trans_v2.y = (1<<2)<<TRANS_Z_FRAC_BITS; z_recip_v2 = 65535; }
                if(trans_v1.y < 0 && trans_v2.y < 0) {  continue; }
                s16 x1 = project_and_adjust_x(trans_v1.x, z_recip_v1);
                s16 x2 = project_and_adjust_x(trans_v2.x, z_recip_v2);

                if(x1 > window_max) {
                    if(trans_v1.y < 0 && trans_v2.y > 0) {
                        x1 = window_min;
                    } else {
                    #ifdef DEBUG_PORTAL_CLIP
                        walls_frustum_culled++;
                        KLog_S1("portal right frustum culled after near clip?: ", portal_idx);
                        KLog_S2("left: ", x1, " right: ", x2);
                    #endif
                        continue;
                    }
                }
                if(x2 <= window_min) {
                    if(trans_v2.y < 0 && trans_v1.y > 0) {
                        //goto submit_anyway;
                        x2 = window_max;
                    } else {
                    #ifdef DEBUG_PORTAL_CLIP
                    walls_frustum_culled++;
                    KLog_S1("portal left frustum culled after near clip?: ", portal_idx);
                    #endif
                    continue;
                    }
                }
                if(x1 >= x2) {
                    #ifdef DEBUG_PORTAL_CLIP
                    post_project_backfacing_walls++;
                    KLog_S1("portal backface culled after near clip?: ", portal_idx);
                    #endif
                    continue;
                }
                neighbor_floor_height = get_sector_floor_height(portal_sector);
                neighbor_ceil_height = get_sector_ceil_height(portal_sector);
                if (neighbor_ceil_height > floor_height && neighbor_floor_height < ceil_height) {
                    visit_graph(src_sector, portal_sector, window_min, window_max, cur_frame, depth+1);
                    //add_new_sector_to_queue(window_min, window_max, portal_sector);
                }

            }

            continue;
        }

        s16 trans_v1_z_fix = trans_v1.y;
        s32 trans_v1_z_int = trans_v1_z_fix>>TRANS_Z_FRAC_BITS;
        s16 trans_v2_z_fix = trans_v2.y;
        s32 trans_v2_z_int = trans_v2_z_fix>>TRANS_Z_FRAC_BITS;

        s16 max_z_int = max(trans_v1_z_int, trans_v2_z_int);
        if(0) { //if(debug) {
            KLog("debug project v1 x");
        }
        s16 x1 = project_and_adjust_x(trans_v1.x, z_recip_v1);
        if(0) { //if(debug) {
            KLog("debug project v2 x");
        }
        s16 x2 = project_and_adjust_x(trans_v2.x, z_recip_v2);
        s16 beginx = x1;
        if(beginx <= window_min) {
            beginx = window_min;
        }

        s16 endx = x2;
        if(endx >= window_max) {
            endx = window_max;
        }


        if (x1 > window_max) {
                #ifdef DEBUG_PORTAL_CLIP
                walls_frustum_culled++;
                if(is_portal) {
                    KLog_S1("portal right frustum culled: ", portal_idx);
                }
                #endif
                if(is_portal && clipped == LEFT_CLIPPED && x2 >= window_min) {
                    #ifdef DEBUG_PORTAL_CLIP
                        KLog_S1("portal right frustum culled but still enqueued: ", portal_idx);
                    #endif
                    visit_graph(src_sector, portal_sector, window_min, endx, cur_frame, depth+1);
                }
                continue;
            } else if (x2 <= window_min) {
                #ifdef DEBUG_PORTAL_CLIP
                    walls_frustum_culled++;
                    if(is_portal) {
                        KLog_S1("portal left frustum culled: ", portal_idx);
                    }
                #endif
                if(is_portal && clipped == RIGHT_CLIPPED && x1 < window_max) {
                    #ifdef DEBUG_PORTAL_CLIP
                        KLog_S2("portal left frustum culled but still enqueued: ", portal_idx, " to sector: ", portal_sector);
                    #endif
                    visit_graph(src_sector, portal_sector, beginx, window_max, cur_frame, depth+1);
                }

                continue;
            } else {
                #ifdef DEBUG_PORTAL_CLIP
                if(is_portal) {
                    KLog_S2("portal not frustum culled: ", portal_idx, " to sector: ", portal_sector);
                }
                #endif
            }

        if(x1 >= x2) {
            #ifdef DEBUG_PORTAL_CLIP
            post_project_backfacing_walls++;
            if(is_portal) {
                KLog_S1("portal backface culled: ", portal_idx);
                KLog_S2("coords x1: ", x1, "x2: ", x2);
            }
            #endif
            continue;
        } else {
            #ifdef DEBUG_PORTAL_CLIP
            if(is_portal) {
                KLog_S1("portal not backface culled: ", portal_idx);
            }
            #endif
        }

        int recur = 0;

        s16 x1_ytop, x1_ybot;
        s16 x2_ytop, x2_ybot;

        s16 x1_pegged, x2_pegged;


        if(is_portal) {
            u8 neighbor_sector_type = cur_portal_map->sector_types[portal_sector];
            s16 neighbor_ceil_color = get_sector_floor_color(portal_sector);
            s16 neighbor_floor_color = get_sector_floor_color(portal_sector);

            neighbor_floor_height = get_sector_floor_height(portal_sector);
            neighbor_ceil_height = get_sector_ceil_height(portal_sector);


            x1_ybot = project_and_adjust_y_fix(floor_height, z_recip_v1);
            x2_ybot = project_and_adjust_y_fix(floor_height, z_recip_v2);

            x1_ytop = project_and_adjust_y_fix(ceil_height, z_recip_v1);
            x2_ytop = project_and_adjust_y_fix(ceil_height, z_recip_v2);

            if (neighbor_ceil_height > floor_height && neighbor_floor_height < ceil_height) {
                recur = 1;
            }
        } else {

            s16 x2_ybot_floor_height = floor_height;
            s16 x1_ybot_floor_height = floor_height;

            x2_ybot = project_and_adjust_y_fix(x2_ybot_floor_height, z_recip_v2);
            x1_ybot = project_and_adjust_y_fix(x1_ybot_floor_height, z_recip_v1);


            s16 x2_ytop_floor_height = ceil_height;
            s16 x1_ytop_floor_height = ceil_height;


            x2_ytop = project_and_adjust_y_fix(x2_ytop_floor_height, z_recip_v2);
            x1_ytop = project_and_adjust_y_fix(x1_ytop_floor_height, z_recip_v1);

            if(cur_sect_type == DOOR || cur_sect_type == LIFT) {
                // top peg all walls in a door/crusher type sector
                s16 orig_door_height = get_sector_orig_height(sector);
                //s16 orig_height_diff = orig_door_height - neighbor_floor_height;
                x1_pegged = project_and_adjust_y_fix(orig_door_height, z_recip_v1);
                x2_pegged = project_and_adjust_y_fix(orig_door_height, z_recip_v2);
            }

        }


        // draw floor and ceiling
        // check if floor and ceiling are on-screen
        clip_buf* clipping_buffer = NULL;


        int render_forcefield = 0; //(sector == 0 && is_portal && portal_sector == 2) || (sector == 2 && is_portal && portal_sector == 0);
        render_forcefield = 0; //(sector == 6 && is_portal && portal_sector == 7) || (sector == 7 && is_portal && portal_sector == 6);
        int render_glass = 0; //(sector == 0 && is_portal && portal_sector == 1) || (sector == 1 && is_portal && portal_sector == 0);



        if (render_forcefield)  {

            clipping_buffer = alloc_clip_buffer();
            if(clipping_buffer == NULL) {
                die("Out of clipping buffers!");
            }
        }

        u8 tex_idx = map->wall_colors[(portal_idx<<WALL_COLOR_NUM_PARAMS_SHIFT)+WALL_TEXTURE_IDX];
        lit_texture *tex = textures[tex_idx];
        tmap_info.tex = tex;

        if (is_portal) {

            #ifdef DEBUG_PORTAL_CLIP
            KLog_S2("portal in sector: ", sector, " drawn with wall idx: ", i);
            #endif
            // draw step down from ceiling

            if(1) { //neighbor_ceil_height < ceil_height) {

                s16 nx1_ytop = project_and_adjust_y_fix(neighbor_ceil_height, z_recip_v1);
                s16 nx2_ytop = project_and_adjust_y_fix(neighbor_ceil_height, z_recip_v2);
                u8 neighbor_sector_type = cur_portal_map->sector_types[portal_sector];

                if(neighbor_sector_type == DOOR) {
                    tmap_info.tex = &sci_fi_door_texture;

                    s16 orig_door_height = get_sector_orig_height(portal_sector);
                    s16 orig_height_diff = orig_door_height - neighbor_floor_height;
                    x1_pegged = project_and_adjust_y_fix(neighbor_ceil_height+orig_height_diff, z_recip_v1);
                    x2_pegged = project_and_adjust_y_fix(neighbor_ceil_height+orig_height_diff, z_recip_v2);
                }

                u8 upper_color = map->wall_colors[(portal_idx<<WALL_COLOR_NUM_PARAMS_SHIFT)+WALL_HIGH_COLOR_IDX];
                // draw step from ceiling
                //draw_upper_step(x1, x1_ytop, nx1_ytop, x2, x2_ytop, nx2_ytop,
                //                z_recip_v1, z_recip_v2,
                //                window_min, window_max, upper_color, light_level, &ceil_params);

                if(neighbor_sector_type == DOOR) {
                    //KLog_S2("nx1_ytop: ", nx1_ytop, "nx2_ytop: ", nx2_ytop);
                    //KLog_S2("x1 door top: ", x1_door_top, "x2 door top: ", x2_door_top);
                    draw_top_pegged_textured_upper_step(x1, x1_ytop, nx1_ytop, x2, x2_ytop, nx2_ytop,
                                                        trans_v1_z_fix, trans_v2_z_fix,
                                                        z_recip_v1, z_recip_v2,
                                                        window_min, window_max,
                                                        light_level, &tmap_info,
                                                        &ceil_params, x1_pegged, x2_pegged);
                } else {
                    draw_upper_step(x1, x1_ytop, nx1_ytop, x2, x2_ytop, nx2_ytop,
                                    z_recip_v1, z_recip_v2,
                                    window_min, window_max, upper_color, light_level, &ceil_params);

                }
            } else {
                draw_ceiling_update_clip(x1, x1_ytop, x2, x2_ytop,
                                        min(z_recip_v1, z_recip_v2),
                                        window_min, window_max, &ceil_params);
            }

            // not sure if this logic is correct
            // if the neighbor's floor is higher than our ceiling we should still draw the lower step, right?
            if(neighbor_floor_height > floor_height) { //} && neighbor_floor_height <= ceil_height) {
                s16 nx1_ybot = project_and_adjust_y_fix(neighbor_floor_height, z_recip_v1);
                s16 nx2_ybot = project_and_adjust_y_fix(neighbor_floor_height, z_recip_v2);
                u8 neighbor_sector_type = cur_portal_map->sector_types[portal_sector];

                if(neighbor_sector_type == LIFT) {
                    tmap_info.tex = &sci_fi_door_texture;

                    s16 orig_lift_height = get_sector_orig_height(portal_sector);
                    s16 orig_height_diff = neighbor_ceil_height - orig_lift_height;
                    x1_pegged = project_and_adjust_y_fix(neighbor_floor_height-orig_height_diff, z_recip_v1);
                    x2_pegged = project_and_adjust_y_fix(neighbor_floor_height-orig_height_diff, z_recip_v2);
                    //KLOG
                }

                u8 lower_color = map->wall_colors[(portal_idx<<WALL_COLOR_NUM_PARAMS_SHIFT)+WALL_LOW_COLOR_IDX];

                if(neighbor_sector_type == LIFT) {

                    draw_bottom_pegged_textured_lower_step(x1, x1_ybot, nx1_ybot, x2, x2_ybot, nx2_ybot,
                                                        trans_v1_z_fix, trans_v2_z_fix,
                                                        z_recip_v1, z_recip_v2,
                                                        window_min, window_max,
                                                        light_level, &tmap_info,
                                                        &floor_params, x1_pegged, x2_pegged);
                } else {
                    // draw step from floor
                    draw_lower_step(x1, x1_ybot, nx1_ybot, x2, x2_ybot, nx2_ybot,
                                    z_recip_v1, z_recip_v2,
                                    window_min, window_max, lower_color, light_level, &floor_params);
                }
            } else {
                draw_floor_update_clip(x1, x1_ybot, x2, x2_ybot,
                                    min(z_recip_v1, z_recip_v2),
                                    window_min, window_max, &floor_params);
            }

            //if(render_forcefield || render_glass) {
            //    copy_2d_buffer(window_min, window_max, clipping_buffer);
            //}

            if(recur) {
                visit_graph(src_sector, portal_sector,
                            ((clipped == LEFT_CLIPPED) ? window_min : beginx),
                            ((clipped == RIGHT_CLIPPED) ? window_max : endx),
                            cur_frame, depth+1);

            }

        } else {
            #ifdef DEBUG_PORTAL_CLIP
            KLog_S2("wall in sector: ", sector, " drawn with wall idx: ", i);
            #endif
            if(cur_sect_type == DOOR) {
                draw_top_pegged_wall(x1, x1_ytop, x1_ybot, x2, x2_ytop, x2_ybot,
                                     trans_v1_z_fix, trans_v2_z_fix,
                                     z_recip_v1, z_recip_v2,
                                     window_min, window_max,
                                     light_level, &tmap_info,
                                     &floor_params, &ceil_params,
                                     x1_pegged, x2_pegged);
            } else if (cur_sect_type == LIFT) {
                draw_bot_pegged_wall(x1, x1_ytop, x1_ybot, x2, x2_ytop, x2_ybot,
                                     trans_v1_z_fix, trans_v2_z_fix,
                                     z_recip_v1, z_recip_v2,
                                     window_min, window_max,
                                     light_level, &tmap_info,
                                     &floor_params, &ceil_params,
                                     x1_pegged, x2_pegged);
            } else {
                draw_wall(x1, x1_ytop, x1_ybot, x2, x2_ytop, x2_ybot,
                            trans_v1_z_fix, trans_v2_z_fix,
                            z_recip_v1, z_recip_v2,
                            window_min, window_max,
                            light_level, &tmap_info,
                            &floor_params, &ceil_params);
            }
        }
        //nwalls++;
        // after this point, draw some sprites :^)

        
        //if (render_forcefield) {
        //    uint16_t col = (RED_IDX<<4)|LIGHT_GREEN_IDX;
        //    draw_forcefield(x1, x2, window_min, window_max, clipping_buffer, col);
        //    free_clip_buffer(clipping_buffer);
        //}
    }


    /*
    if(objects_in_sect != NULL) {
        object* cur_obj = objects_in_sect;
        while(cur_obj != NULL) {
            object_pos pos = cur_obj->pos;
            s16 flr_height = get_sector_floor_height(sector);
            Vect2D_s16 trans_pos = transform_map_vert_16(fix32ToInt(pos.x), fix32ToInt(pos.y));
            
            u16 z_recip = z_recip_table_16[trans_pos.y>>TRANS_Z_FRAC_BITS];

            if(trans_pos.y > 0) {

                s16 left_x = project_and_adjust_x(trans_pos.x-6, z_recip);
                s16 right_x = project_and_adjust_x(trans_pos.x+6, z_recip);

                object_template type = object_types[cur_obj->object_type];

                s16 top_y = project_and_adjust_y_fix(pos.z+type.from_floor_draw_offset+type.height, z_recip);
                s16 bot_y = project_and_adjust_y_fix(pos.z+type.from_floor_draw_offset, z_recip);

                draw_masked(left_x,top_y, bot_y,
                        right_x, top_y, bot_y,
                        window_min, window_max,
                        obj_clip_buf,
                        type.sprite_col);
            }
            break;
            //cur_obj = cur_obj->next;
        }
        free_clip_buffer(obj_clip_buf);
    }
    */
}



void pvs_scan(u16 src_sector, s16 window_min, s16 window_max, u32 cur_frame) {
    portal_map* map = (portal_map*)cur_portal_map;
    u16 raw_pvs_offset = map->pvs[src_sector<<PVS_SHIFT];
    u16 num_sect_pvs_groups = map->pvs[(src_sector<<PVS_SHIFT)+1];


    for(int i = 0; i < num_sect_pvs_groups; i++) {
        u16 sector = map->raw_pvs[raw_pvs_offset++];

        s8 light_level = get_sector_light_level(sector);
        s16 floor_height = get_sector_floor_height(sector);
        s16 ceil_height = get_sector_ceil_height(sector);
        s16 floor_col = get_sector_floor_color(sector);
        s16 ceil_col = get_sector_ceil_color(sector);
        s16 rel_floor_height = (floor_height>>4) - (cur_player_pos.z>>(FIX32_FRAC_BITS));
        s16 rel_ceil_height = (ceil_height>>4) - (cur_player_pos.z>>(FIX32_FRAC_BITS));
        light_params ceil_params, floor_params;
        cache_ceil_light_params(rel_ceil_height, ceil_col, light_level, &ceil_params);
        cache_floor_light_params(rel_floor_height, floor_col, light_level, &floor_params);

        u16 num_walls = map->raw_pvs[raw_pvs_offset++];

        for(int j = 0; j < num_walls; j++) {
            u16 wall_idx = map->raw_pvs[raw_pvs_offset++];
            u16 portal_idx = wall_idx - sector;
            s16 portal_sector = map->portals[portal_idx];

            u8 is_portal = (portal_sector != -1);

            u16 v1_idx = map->walls[wall_idx];
            u16 v2_idx = map->walls[wall_idx+1];

            vertex v1 = map->vertexes[v1_idx];
            vertex v2 = map->vertexes[v2_idx];

            u32 wall_len = getApproximatedDistance(v2.x - v1.x, v2.y - v1.y);
            Vect2D_s16 trans_v1 = transform_map_vert_16(v1.x, v1.y);
            Vect2D_s16 trans_v2 = transform_map_vert_16(v2.x, v2.y);


            texmap_params tmap_info = {.tex = &sci_fi_wall_texture};

            clip_result clipped = clip_map_vertex_16(&trans_v1, &trans_v2, &tmap_info, wall_len);

            if(clipped == OFFSCREEN) {
                continue;
            }

            s16 trans_v1_z_fix = trans_v1.y;
            s16 trans_v2_z_fix = trans_v2.y;
            u16 z_recip_v1 = z_recip_table_16[trans_v1.y>>TRANS_Z_FRAC_BITS];
            u16 z_recip_v2 = z_recip_table_16[trans_v2.y>>TRANS_Z_FRAC_BITS];

            s16 x1 = project_and_adjust_x(trans_v1.x, z_recip_v1);
            s16 x2 = project_and_adjust_x(trans_v2.x, z_recip_v2);

            s16 x1_ytop, x1_ybot;
            s16 x2_ytop, x2_ybot;

            x1_ybot = project_and_adjust_y_fix(floor_height, z_recip_v1);
            x2_ybot = project_and_adjust_y_fix(floor_height, z_recip_v2);


            x1_ytop = project_and_adjust_y_fix(ceil_height, z_recip_v1);
            x2_ytop = project_and_adjust_y_fix(ceil_height, z_recip_v2);

            if(x1 > window_max) {
                continue;
            } else if (x2 <= window_min) {
                continue;
            }

            if(x1 >= x2) {
                continue;
            }


            if(is_portal) {
                u8 neighbor_sector_type = cur_portal_map->sector_types[portal_sector];
                s16 neighbor_ceil_color = get_sector_floor_color(portal_sector);
                s16 neighbor_floor_color = get_sector_floor_color(portal_sector);

                s16 neighbor_floor_height = get_sector_floor_height(portal_sector);
                s16 neighbor_ceil_height = get_sector_ceil_height(portal_sector);


                // draw step down from ceiling

                if(neighbor_ceil_height < ceil_height) {

                    s16 nx1_ytop = project_and_adjust_y_fix(neighbor_ceil_height, z_recip_v1);
                    s16 nx2_ytop = project_and_adjust_y_fix(neighbor_ceil_height, z_recip_v2);

                    u8 upper_color = map->wall_colors[(portal_idx<<WALL_COLOR_NUM_PARAMS_SHIFT)+WALL_HIGH_COLOR_IDX];
                    if(neighbor_sector_type == DOOR) {
                        tmap_info.tex = &sci_fi_door_texture;

                        s16 orig_door_height = get_sector_orig_height(portal_sector);
                        s16 orig_height_diff = orig_door_height - neighbor_floor_height;
                        s16 x1_pegged = project_and_adjust_y_fix(neighbor_ceil_height+orig_height_diff, z_recip_v1);
                        s16 x2_pegged = project_and_adjust_y_fix(neighbor_ceil_height+orig_height_diff, z_recip_v2);

                        draw_top_pegged_textured_upper_step(x1, x1_ytop, nx1_ytop, x2, x2_ytop, nx2_ytop,
                                                            trans_v1_z_fix, trans_v2_z_fix,
                                                            z_recip_v1, z_recip_v2,
                                                            window_min, window_max,
                                                            light_level, &tmap_info,
                                                            &ceil_params, x1_pegged, x2_pegged);
                    } else {
                        draw_upper_step(x1, x1_ytop, nx1_ytop, x2, x2_ytop, nx2_ytop,
                                        z_recip_v1, z_recip_v2,
                                        window_min, window_max, upper_color, light_level, &ceil_params);

                    }
                } else {
                    draw_ceiling_update_clip(x1, x1_ytop, x2, x2_ytop,
                                            min(z_recip_v1, z_recip_v2),
                                            window_min, window_max, &ceil_params);
                }

                // not sure if this logic is correct
                // if the neighbor's floor is higher than our ceiling we should still draw the lower step, right?
                if(neighbor_floor_height > floor_height) { //} && neighbor_floor_height <= ceil_height) {
                    s16 nx1_ybot = project_and_adjust_y_fix(neighbor_floor_height, z_recip_v1);
                    s16 nx2_ybot = project_and_adjust_y_fix(neighbor_floor_height, z_recip_v2);
                    u8 neighbor_sector_type = cur_portal_map->sector_types[portal_sector];

                    u8 lower_color = map->wall_colors[(portal_idx<<WALL_COLOR_NUM_PARAMS_SHIFT)+WALL_LOW_COLOR_IDX];
                    if(neighbor_sector_type == LIFT) {
                        tmap_info.tex = &sci_fi_door_texture;

                        s16 orig_lift_height = get_sector_orig_height(portal_sector);
                        s16 orig_height_diff = neighbor_ceil_height - orig_lift_height;
                        s16 x1_pegged = project_and_adjust_y_fix(neighbor_floor_height-orig_height_diff, z_recip_v1);
                        s16 x2_pegged = project_and_adjust_y_fix(neighbor_floor_height-orig_height_diff, z_recip_v2);

                        draw_bottom_pegged_textured_lower_step(x1, x1_ybot, nx1_ybot, x2, x2_ybot, nx2_ybot,
                                                            trans_v1_z_fix, trans_v2_z_fix,
                                                            z_recip_v1, z_recip_v2,
                                                            window_min, window_max,
                                                            light_level, &tmap_info,
                                                            &floor_params, x1_pegged, x2_pegged);
                    } else {
                        // draw step from floor
                        draw_lower_step(x1, x1_ybot, nx1_ybot, x2, x2_ybot, nx2_ybot,
                                        z_recip_v1, z_recip_v2,
                                        window_min, window_max, lower_color, light_level, &floor_params);
                    }
                } else {
                    draw_floor_update_clip(x1, x1_ybot, x2, x2_ybot,
                                        min(z_recip_v1, z_recip_v2),
                                        window_min, window_max, &floor_params);
                }


            } else {


                draw_wall(
                    x1, x1_ytop, x1_ybot,
                    x2, x2_ytop, x2_ybot,
                    trans_v1_z_fix, trans_v2_z_fix, z_recip_v1, z_recip_v2,
                    window_min, window_max,
                    light_level, &tmap_info, &floor_params, &ceil_params);

                if(x1 <= window_min && x2 > window_min) {
                    window_min = max(0, x2);
                    if(window_min > RENDER_WIDTH) {
                        return;
                    }
                }
                if(x2 >= window_max && x1 < window_max) {
                    window_max = min(x1, RENDER_WIDTH);
                    if(window_max < 0) {
                        return;
                    }
                }
                if(window_min >= window_max) {
                    return;
                }

            }
        }
    }
}



void pvs_scan_with_objects(u16 src_sector, s16 window_min, s16 window_max, u32 cur_frame) {
    portal_map* map = (portal_map*)cur_portal_map;
    u16 raw_pvs_offset = map->pvs[src_sector<<PVS_SHIFT];
    u16 num_sect_pvs_groups = map->pvs[(src_sector<<PVS_SHIFT)+1];

    typedef enum {
        WALL,
        SPRITE,
    } draw_obj_type;

    typedef struct {
        u16 sector;
        u16 wall_idx;
    } draw_line;

    typedef struct {
        object* obj;
    } draw_spr;

    typedef struct {
        draw_obj_type typ;
        union {
            draw_spr spr;
            draw_line wall;
        };
    } draw_obj;

    draw_obj draw_objs[128];
    int num_draw_objs;

    for(int i = 0; i < num_sect_pvs_groups; i++) {
        u16 sector = map->raw_pvs[raw_pvs_offset++];
        u16 num_walls = map->raw_pvs[raw_pvs_offset++];
        
        for(int j = 0; j < num_walls && num_draw_objs < 128; j++) {
            u16 wall_idx = map->raw_pvs[raw_pvs_offset++];
            draw_objs[num_draw_objs].typ = WALL;
            draw_objs[num_draw_objs].wall.sector = sector;
            draw_objs[num_draw_objs].wall.wall_idx = wall_idx;
            num_draw_objs++;
        }
    }

    for(int i = 0; i < num_draw_objs; i++) {
        draw_obj obj = draw_objs[i];
        if(obj.typ == SPRITE) { 
            continue;
        }

        u16 sector = obj.wall.sector;

        s8 light_level = get_sector_light_level(sector);
        s16 floor_height = get_sector_floor_height(sector);
        s16 ceil_height = get_sector_ceil_height(sector);
        s16 floor_col = get_sector_floor_color(sector);
        s16 ceil_col = get_sector_ceil_color(sector);
        s16 rel_floor_height = (floor_height>>4) - (cur_player_pos.z>>(FIX32_FRAC_BITS));
        s16 rel_ceil_height = (ceil_height>>4) - (cur_player_pos.z>>(FIX32_FRAC_BITS));
        light_params ceil_params, floor_params;
        cache_ceil_light_params(rel_ceil_height, ceil_col, light_level, &ceil_params);
        cache_floor_light_params(rel_floor_height, floor_col, light_level, &floor_params);


        u16 wall_idx = obj.wall.wall_idx;
        u16 portal_idx = wall_idx - sector;
        s16 portal_sector = map->portals[portal_idx];

        u8 is_portal = (portal_sector != -1);

        u16 v1_idx = map->walls[wall_idx];
        u16 v2_idx = map->walls[wall_idx+1];

        vertex v1 = map->vertexes[v1_idx];
        vertex v2 = map->vertexes[v2_idx];

        u32 wall_len = getApproximatedDistance(v2.x - v1.x, v2.y - v1.y);
        Vect2D_s16 trans_v1 = transform_map_vert_16(v1.x, v1.y);
        Vect2D_s16 trans_v2 = transform_map_vert_16(v2.x, v2.y);


        texmap_params tmap_info = {.tex = &sci_fi_wall_texture};

        clip_result clipped = clip_map_vertex_16(&trans_v1, &trans_v2, &tmap_info, wall_len);

        if(clipped == OFFSCREEN) {
            continue;
        }

        s16 trans_v1_z_fix = trans_v1.y;
        s16 trans_v2_z_fix = trans_v2.y;
        u16 z_recip_v1 = z_recip_table_16[trans_v1.y>>TRANS_Z_FRAC_BITS];
        u16 z_recip_v2 = z_recip_table_16[trans_v2.y>>TRANS_Z_FRAC_BITS];

        s16 x1 = project_and_adjust_x(trans_v1.x, z_recip_v1);
        s16 x2 = project_and_adjust_x(trans_v2.x, z_recip_v2);

        s16 x1_ytop, x1_ybot;
        s16 x2_ytop, x2_ybot;

        x1_ybot = project_and_adjust_y_fix(floor_height, z_recip_v1);
        x2_ybot = project_and_adjust_y_fix(floor_height, z_recip_v2);


        x1_ytop = project_and_adjust_y_fix(ceil_height, z_recip_v1);
        x2_ytop = project_and_adjust_y_fix(ceil_height, z_recip_v2);

        if(x1 > window_max) {
            continue;
        } else if (x2 <= window_min) {
            continue;
        }

        if(x1 >= x2) {
            continue;
        }


        if(is_portal) {
            u8 neighbor_sector_type = cur_portal_map->sector_types[portal_sector];
            s16 neighbor_ceil_color = get_sector_floor_color(portal_sector);
            s16 neighbor_floor_color = get_sector_floor_color(portal_sector);

            s16 neighbor_floor_height = get_sector_floor_height(portal_sector);
            s16 neighbor_ceil_height = get_sector_ceil_height(portal_sector);


            // draw step down from ceiling

            if(neighbor_ceil_height < ceil_height) {

                s16 nx1_ytop = project_and_adjust_y_fix(neighbor_ceil_height, z_recip_v1);
                s16 nx2_ytop = project_and_adjust_y_fix(neighbor_ceil_height, z_recip_v2);

                u8 upper_color = map->wall_colors[(portal_idx<<WALL_COLOR_NUM_PARAMS_SHIFT)+WALL_HIGH_COLOR_IDX];
                if(neighbor_sector_type == DOOR) {
                    tmap_info.tex = &sci_fi_door_texture;

                    s16 orig_door_height = get_sector_orig_height(portal_sector);
                    s16 orig_height_diff = orig_door_height - neighbor_floor_height;
                    s16 x1_pegged = project_and_adjust_y_fix(neighbor_ceil_height+orig_height_diff, z_recip_v1);
                    s16 x2_pegged = project_and_adjust_y_fix(neighbor_ceil_height+orig_height_diff, z_recip_v2);

                    draw_top_pegged_textured_upper_step(x1, x1_ytop, nx1_ytop, x2, x2_ytop, nx2_ytop,
                                                        trans_v1_z_fix, trans_v2_z_fix,
                                                        z_recip_v1, z_recip_v2,
                                                        window_min, window_max,
                                                        light_level, &tmap_info,
                                                        &ceil_params, x1_pegged, x2_pegged);
                } else {
                    draw_upper_step(x1, x1_ytop, nx1_ytop, x2, x2_ytop, nx2_ytop,
                                    z_recip_v1, z_recip_v2,
                                    window_min, window_max, upper_color, light_level, &ceil_params);

                }
            } else {
                draw_ceiling_update_clip(x1, x1_ytop, x2, x2_ytop,
                                        min(z_recip_v1, z_recip_v2),
                                        window_min, window_max, &ceil_params);
            }

            // not sure if this logic is correct
            // if the neighbor's floor is higher than our ceiling we should still draw the lower step, right?
            if(neighbor_floor_height > floor_height) { //} && neighbor_floor_height <= ceil_height) {
                s16 nx1_ybot = project_and_adjust_y_fix(neighbor_floor_height, z_recip_v1);
                s16 nx2_ybot = project_and_adjust_y_fix(neighbor_floor_height, z_recip_v2);
                u8 neighbor_sector_type = cur_portal_map->sector_types[portal_sector];

                u8 lower_color = map->wall_colors[(portal_idx<<WALL_COLOR_NUM_PARAMS_SHIFT)+WALL_LOW_COLOR_IDX];
                if(neighbor_sector_type == LIFT) {
                    tmap_info.tex = &sci_fi_door_texture;

                    s16 orig_lift_height = get_sector_orig_height(portal_sector);
                    s16 orig_height_diff = neighbor_ceil_height - orig_lift_height;
                    s16 x1_pegged = project_and_adjust_y_fix(neighbor_floor_height-orig_height_diff, z_recip_v1);
                    s16 x2_pegged = project_and_adjust_y_fix(neighbor_floor_height-orig_height_diff, z_recip_v2);

                    draw_bottom_pegged_textured_lower_step(x1, x1_ybot, nx1_ybot, x2, x2_ybot, nx2_ybot,
                                                        trans_v1_z_fix, trans_v2_z_fix,
                                                        z_recip_v1, z_recip_v2,
                                                        window_min, window_max,
                                                        light_level, &tmap_info,
                                                        &floor_params, x1_pegged, x2_pegged);
                } else {
                    // draw step from floor
                    draw_lower_step(x1, x1_ybot, nx1_ybot, x2, x2_ybot, nx2_ybot,
                                    z_recip_v1, z_recip_v2,
                                    window_min, window_max, lower_color, light_level, &floor_params);
                }
            } else {
                draw_floor_update_clip(x1, x1_ybot, x2, x2_ybot,
                                    min(z_recip_v1, z_recip_v2),
                                    window_min, window_max, &floor_params);
            }


        } else {


            draw_wall(x1, x1_ytop, x1_ybot,
                x2, x2_ytop, x2_ybot,
                trans_v1_z_fix, trans_v2_z_fix, z_recip_v1, z_recip_v2,
                window_min, window_max,
                light_level, &tmap_info, &floor_params, &ceil_params);

            if(x1 <= window_min && x2 > window_min) {
                window_min = max(0, x2);
                if(window_min > RENDER_WIDTH) {
                    return;
                }
            }
            if(x2 >= window_max && x1 < window_max) {
                window_max = min(x1, RENDER_WIDTH);
                if(window_max < 0) {
                    return;
                }
            }
            if(window_min >= window_max) {
                return;
            }

        }
    }
}



void portal_rend(u16 src_sector, u32 cur_frame) {
    #ifdef DEBUG_PORTAL_CLIP
    pre_transform_backfacing_walls = 0;
    walls_frustum_culled = 0;
    portals_frustum_culled = 0;
    post_project_backfacing_walls = 0;
    #endif

    #ifdef H32_MODE
    //visit_graph(src_sector, src_sector, 7, RENDER_WIDTH-7-1, cur_frame, 0);
    #else
    sectors_scanned = 0;
    //clear_light_cache();

    if(cur_portal_map->has_pvs) {
        pvs_scan(src_sector, 0, RENDER_WIDTH, cur_frame);

        //pvs_scan_with_objects(src_sector, 0, RENDER_WIDTH, cur_frame);
    } else {
        visit_graph(src_sector, src_sector, 0, RENDER_WIDTH, cur_frame, 0);
    }
    //KLog_U1("sectors recursively scanned: ", sectors_scanned);
    #endif
    //sectors_scanned = 0;
    //portal_scan(src_sector, 0, RENDER_WIDTH, cur_frame);
    return;
    #ifdef DEBUG_PORTAL_CLIP
    KLog_S1("walls pre-transform backface culled ", pre_transform_backfacing_walls);
    KLog_S1("portals/walls frustum culled ", walls_frustum_culled);
    KLog_S1("portals frustum culled ", portals_frustum_culled);
    KLog_S1("walls post-project backface culled ", post_project_backfacing_walls);
    KLog("-------- frame-end --------");
    #endif

}
