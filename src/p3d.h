/*
 * Copyright (C)  2016  Felix "KoffeinFlummi" Wiegand
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once


#define MAXPOINTS 
#define VERSION70

#define GEOMETRY 10000000000000.0f


struct point {
    float x;
    float y;
    float z;
    uint32_t point_flags;
};

struct triplet {
    float x;
    float y;
    float z;
};

struct pseudovertextable {
    uint32_t points_index;
    uint32_t normals_index;
    float u;
    float v;
};

struct mlod_face {
    uint32_t face_type;
    struct pseudovertextable table[4];
    uint32_t face_flags;
    char texture_name[512];
    char material_name[512];
};

struct odol_lod {
    uint8_t face_type;
#ifdef VERSION70
    uint32_t table[4];
#else
    uint16_t table[4];
#endif
};
    
struct mlod_lod {
    uint32_t num_points;
    uint32_t num_facenormals;
    uint32_t num_faces;
    struct point *points;
    struct triplet *facenormals;
    struct mlod_face *faces;
    float *mass;
    float resolution;
};

struct skeleton {

};

struct model_info {
    float *lod_resolutions;
    uint32_t index;
    float mem_lod_sphere;
    float geo_lod_sphere;
    uint32_t point_flags[3];
    struct triplet offset1;
    uint32_t map_icon_color;
    uint32_t map_selected_color;
    float view_density;
    struct triplet bbox_min;
    struct triplet bbox_max;
    struct triplet centre_of_gravity;
    struct triplet offset2;
    struct triplet cog_offset;
    struct triplet model_mass_vectors[3];
    char thermal_profile2[24];
    bool autocenter;
    bool lock_autocenter;
    bool can_occlude;
    bool can_be_occluded;
    bool allow_animation;
    char unknown_flags[6];
    char thermal_profile[24];
    uint32_t unknown_long;
    struct skeleton skeleton;
    char unknown_byte;
    uint32_t n_floats;
    float mass;
    float mass_reciprocal;
    float alt_mass;
    float alt_mass_reciprocal;
    char unknown_indices[14];
    uint32_t unknown_long_2;
    bool unknown_bool;
    char class_type;
    char destruct_type;
    bool unknown_bool_2;
    uint32_t always_0;
};


int mlod2odol(char *source, char *target);
