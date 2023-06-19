#include <genesis.h>
#include "game.h"
#include "collision.h"
#include "level.h"
#include "math3d.h"

fix32 vxs16(fix16 x0, fix16 y0, fix16 x1, fix16 y1) {
    return x0*y1 - x1*y0;
}

fix32 vxs32(fix32 x0, fix32 y0, fix32 x1, fix32 y1) {
    return fix32Mul(x0, y1) - fix32Mul(x1, y0);
}

#define PointSide16(px,py, x0,y0, x1,y1) vxs16((x1)-(x0), (y1)-(y0), (px)-(x0), (py)-(y0))
#define PointSide32(px,py, x0,y0, x1,y1) vxs32((x1)-(x0), (y1)-(y0), (px)-(x0), (py)-(y0)) 

#define Overlap(a0,a1,b0,b1) (min(a0,a1) <= max(b0,b1) && min(b0,b1) <= max(a0,a1))
#define IntersectBox(x0,y0, x1,y1, x2,y2, x3,y3) (Overlap(x0,x1,x2,x3) && Overlap(y0,y1,y2,y3))



int within_sector(fix32 x, fix32 y, u16 sector) {
    u16 wall_off = sector_wall_offset(sector, (portal_map*)cur_portal_map);
    u16 num_walls = sector_num_walls(sector, (portal_map*)cur_portal_map);

    int sign = 0;
    int got_sign = 0;


    for(int i = 0; i < num_walls; i++) {
        u16 wall_idx = wall_off+i;
        u16 v1_idx = cur_portal_map->walls[wall_idx];
        u16 v2_idx = cur_portal_map->walls[wall_idx+1];
        vertex v1 = cur_portal_map->vertexes[v1_idx];
        vertex v2 = cur_portal_map->vertexes[v2_idx];
        
        if(point_sign_int_vert(x, y, v1.x, v1.y, v2.x, v2.y)) {
            return 0;
        }
        
        // code for -1,0,1 point sign
        /*
        int cur_sign = point_sign_int_vert(x, y, v1.x, v1.y, v2.x, v2.y);
        if(got_sign) {
         if(sign != cur_sign) {    
            return 0;
        }
         
        } else {
            sign = cur_sign;
            got_sign = 1;
        }
        */
    }

    return 1;
}


s32 sq_shortest_dist_to_point(fix32 px, fix32 py, vertex a, vertex b) {
    s16 dx = b.x - a.x;
    s16 dy = b.y - a.y;

    s32 dr2 = muls_16_by_16(dx, dx);
    dr2 += muls_16_by_16(dy, dy);
    fix32 fixAx = intToFix32(a.x); // 22.10
    fix32 fixAy = intToFix32(a.y);  

    fix32 lerp = ((px - fixAx) * dx + (py - fixAy) * dy) / dr2; // 22.10 fractional bits here
    if (lerp < 0) {
        lerp = 0;
    } else if (lerp > FIX32(1)) {
        lerp = FIX32(1);
    }

    fix32 testX = lerp * dx + fixAx;
    fix32 testY = lerp * dy + fixAy;

    fix32 _dx = testX - px;
    fix32 _dy = testY - py;
    fix32 square_dist = fix32Mul(_dx,_dx) + fix32Mul(_dy,_dy);
    return square_dist>>FIX32_FRAC_BITS;
}

int within_sector_radius(fix32 x, fix32 y, fix32 z, u32 radius_sqr, u16 sector) {    
   
    
    portal_map* map = (portal_map*)cur_portal_map;
    u16 cur_sector_group = sector_group(sector, map);
    s16 cur_ceil_height = get_sector_group_ceil_height(cur_sector_group);
    s16 cur_floor_height = get_sector_group_floor_height(cur_sector_group);
    //s16 cur_player_z = (cur_player_pos.z - FIX32(50)) >> (FIX32_FRAC_BITS-4);
    u16 wall_off = sector_wall_offset(sector, map);
    u16 portal_off = sector_portal_offset(sector, map);
    u16 num_walls = sector_num_walls(sector, map);
    int maybe_inside = within_sector(x, y, sector);
    if(!maybe_inside) { return 0; }


    //s32 dmax;
    //s32 dmin;
    //int got_dist = 0;


     
    //if(radius_sqr > dmin) { return 0; }

    // otherwise, it might be inside.

    // now check against walls

    // if the wall is a portal, skip it

    // try disabling this for now
    
    for(int i = 0; i < num_walls; i++) {
        u16 wall_idx = wall_off+i;
        u16 portal_idx = portal_off+i;
        s16 portal_sect = cur_portal_map->portals[portal_idx];        
        u8 is_portal = portal_sect != -1;
        if(is_portal) {
            u16 portal_sect_group = sector_group(portal_sect, map);
            s16 neighbor_floor_height = get_sector_group_floor_height(portal_sect_group);
            s16 neighbor_ceil_height = get_sector_group_ceil_height(portal_sect_group);
            u8 closed_door = (neighbor_ceil_height - cur_floor_height) <= (60 << 4);
            //u8 lift_too_high = (neighbor_floor_height - cur_floor_height) >= (40 << 4);
            u8 lift_too_high = (neighbor_floor_height - z) >= (40 << 4);
            if(closed_door) {
                //KLog("door is closed! 2");
            } else if(lift_too_high) {
                //KLog("lift is too high! 2");
            } else {           
                continue;
            }
        }
        u16 v1_idx = cur_portal_map->walls[wall_idx];
        u16 v2_idx = cur_portal_map->walls[wall_idx+1];
        vertex v1 = cur_portal_map->vertexes[v1_idx];
        vertex v2 = cur_portal_map->vertexes[v2_idx];
        fix32 dst = sq_shortest_dist_to_point(x, y, v1, v2);
        if(radius_sqr > dst) {
            return 0;
        }
    }
    
    

    return 1;

}


collision_result check_for_collision(fix32 curx, fix32 cury, fix32 newx, fix32 newy, u16 cur_sector) {
    if(within_sector(newx, cury, cur_sector)) {
        // no x collision, just move x 
        curx = newx;
    } else {

        // check if traversed to new sector
        u16 portal_off = sector_portal_offset(cur_sector, (portal_map*)cur_portal_map);
        u16 num_walls = sector_num_walls(cur_sector, (portal_map*)cur_portal_map);
        for(int i = 0; i < num_walls; i++) {
            u16 portal_idx = portal_off+i;
            s16 portal_sect = cur_portal_map->portals[portal_idx];
            if(portal_sect == -1) {
                continue;
            }

            if(within_sector(newx, cury, portal_sect)) {
                cur_sector = portal_sect;
                curx = newx;
                break;
            }
        }
        // if we didn't move to a new sector, curx hasn't been changed

    }

    if(within_sector(curx, newy, cur_sector)) {
        cury = newy;
    } else {
        // check if traversed to new sector
        u16 portal_off = sector_portal_offset(cur_sector, (portal_map*)cur_portal_map);
        u16 num_walls = sector_num_walls(cur_sector, (portal_map*)cur_portal_map);
        for(int i = 0; i < num_walls; i++) {
            u16 portal_idx = portal_off+i;
            s16 portal_sect = cur_portal_map->portals[portal_idx];
            if(portal_sect == -1) {
                //continue;
                cury = newy;
                break;
            }

            if(within_sector(curx, newy, portal_sect)) {
                cur_sector = portal_sect;
                cury = newy;
                break;
            }
        }
        // if we didn't move to a new sector, curx hasn't been changed
    }

    collision_result res;
    res.new_sector = cur_sector;
    res.pos.x = newx; //curx;
    res.pos.y = newy; //cury;

    return res;

}

collision_result check_for_collision_radius(fix32 curx, fix32 cury, fix32 cur_z, fix32 newx, fix32 newy, u32 radius_sqr, u16 cur_sector) {
    portal_map* map = (portal_map*)cur_portal_map;
    u16 cur_sector_group = sector_group(cur_sector, map);
    s16 cur_ceil_height = get_sector_group_ceil_height(cur_sector_group);
    s16 cur_floor_height = get_sector_group_floor_height(cur_sector_group);

    
    //s16 new_x_int = newx>>FIX32_FRAC_BITS;
    //s16 new_y_int = newy>>FIX32_FRAC_BITS;

    u16 wall_off = sector_wall_offset(cur_sector, map);
    u16 portal_off = sector_portal_offset(cur_sector, map);
    u16 num_walls = sector_num_walls(cur_sector, map);
    if(curx == newx) { //} || within_sector_radius(newx, cury, cur_z, radius_sqr, cur_sector)) {
        // no x collision, just move x 
        curx = newx;
    } else {
        // check if traversed to new sector  
        u8 crossed_line_x = 0;
        for(int i = 0; i < num_walls; i++) {

            // check if we've gone over this line
            u16 wall_idx = wall_off+i;
            u16 v1_idx = cur_portal_map->walls[wall_idx];
            u16 v2_idx = cur_portal_map->walls[wall_idx+1];
            vertex v1 = cur_portal_map->vertexes[v1_idx];
            vertex v2 = cur_portal_map->vertexes[v2_idx];
            int over_line = point_sign_int_vert(newx, cury, v1.x, v1.y, v2.x, v2.y);
            
            if(over_line) {
                crossed_line_x = 1;

                u16 portal_idx = portal_off+i;
                s16 portal_sect = cur_portal_map->portals[portal_idx];
                if(portal_sect == -1) {
                    continue;
                }
                if(within_sector(newx, cury, portal_sect)) {
                    u16 portal_sect_group = sector_group(portal_sect, map);
                    s16 neighbor_floor_height = get_sector_group_floor_height(portal_sect_group);
                    s16 neighbor_ceil_height = get_sector_group_ceil_height(portal_sect_group);
                    u8 closed_door = (neighbor_ceil_height - cur_floor_height) <= (60 << 4);

                    u8 floor_too_high = (neighbor_floor_height - cur_z) >= (60 << 4);
                    u8 floor_too_low = 0;//(cur_z - neighbor_floor_height) >= (40 << 4);

                    if(closed_door || floor_too_high || floor_too_low) {
                    } else {                
                        cur_sector = portal_sect;
                        curx = newx;
                    }
                    break;
                }
            }
        }
        if(!crossed_line_x) {
            curx = newx;
        }
    }

    if(cury == newy) {// || within_sector_radius(curx, newy, cur_z, radius_sqr, cur_sector)) {
        cury = newy;
    } else {
        // check if traversed to new sector
        u8 crossed_line_y = 0;

        for(int i = 0; i < num_walls; i++) {
            // check if we've gone over this line
            u16 wall_idx = wall_off+i;
            u16 v1_idx = cur_portal_map->walls[wall_idx];
            u16 v2_idx = cur_portal_map->walls[wall_idx+1];
            vertex v1 = cur_portal_map->vertexes[v1_idx];
            vertex v2 = cur_portal_map->vertexes[v2_idx];
            int over_line = point_sign_int_vert(curx, newy, v1.x, v1.y, v2.x, v2.y);
            
            if(over_line) {
                
                crossed_line_y = 1;
                u16 portal_idx = portal_off+i;
                s16 portal_sect = cur_portal_map->portals[portal_idx];
                if(portal_sect == -1) {
                    continue;
                }
                if(within_sector(curx, newy, portal_sect)) {
                
                    
                    u16 portal_sect_group = sector_group(portal_sect, map);
                    s16 neighbor_floor_height = get_sector_group_floor_height(portal_sect_group);
                    s16 neighbor_ceil_height = get_sector_group_ceil_height(portal_sect_group);
                    u8 closed_door = (neighbor_ceil_height - cur_floor_height) <= (60 << 4);
                    u8 floor_too_high = (neighbor_floor_height - cur_z) >= (60 << 4);
                    u8 floor_too_low = 0;//(cur_z - neighbor_floor_height) >= (40 << 4);

                    if(closed_door || floor_too_high || floor_too_low) {
                    } else {                           
                        cur_sector = portal_sect;
                        cury = newy;
                    }
                    break;
                }
            }
        }
        if(!crossed_line_y) {
            cury = newy;
        }
    }

    collision_result res;
    res.new_sector = cur_sector;
    res.pos.x = curx;
    res.pos.y = cury;

    return res;

}


u16 find_sector(player_pos cur_player_pos) {
    // test current sector first
    
    u16 orig_sector = cur_player_pos.cur_sector;
    if(within_sector(cur_player_pos.x, cur_player_pos.y, orig_sector)) {
        return orig_sector;
    }

    // otherwise test connected sectors
    u16 orig_sector_num_walls = sector_num_walls(orig_sector, (portal_map*)cur_portal_map);
    u16 portal_offset = sector_portal_offset(orig_sector, (portal_map*)cur_portal_map);
    for(int i = 0; i < orig_sector_num_walls; i++) {
        s16 dest_sector = cur_portal_map->portals[portal_offset+i];
        if(dest_sector == -1) {
            continue;
        }
        if(within_sector(cur_player_pos.x, cur_player_pos.y, dest_sector)) {
            return dest_sector;
        }
    }    
    
    //for(int i = 0; i < cur_portal_map->num_sectors; i++) {
    //    if(in_sector(cur_player_pos, i)) {
    //        return i;
    //    }
    //}

    return cur_player_pos.cur_sector;

}
