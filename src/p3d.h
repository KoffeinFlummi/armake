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


//#define VERSION70

#define MAXTEXTURES 128
#define MAXMATERIALS 128
#define MAXPROPERTIES 128

#define LOD_GRAPHICAL                                  999.9f
#define LOD_VIEW_GUNNER                               1000.0f
#define LOD_VIEW_PILOT                                1100.0f
#define LOD_VIEW_CARGO                                1200.0f
#define LOD_SHADOW_STENCIL                           10000.0f
#define LOD_SHADOW_STENCIL_2                         10010.0f
#define LOD_SHADOW_VOLUME                            11000.0f
#define LOD_SHADOW_VOLUME_2                          11010.0f
#define LOD_GEOMETRY                        10000000000000.0f
#define LOD_MEMORY                        1000000000000000.0f
#define LOD_LAND_CONTACT                  2000000000000000.0f
#define LOD_ROADWAY                       3000000000000000.0f
#define LOD_PATHS                         4000000000000000.0f
#define LOD_HITPOINTS                     5000000000000000.0f
#define LOD_VIEW_GEOMETRY                 6000000000000000.0f
#define LOD_FIRE_GEOMETRY                 7000000000000000.0f
#define LOD_VIEW_CARGO_GEOMETRY           8000000000000000.0f
#define LOD_VIEW_CARGO_FIRE_GEOMETRY      9000000000000000.0f
#define LOD_VIEW_COMMANDER               10000000000000000.0f
#define LOD_VIEW_COMMANDER_GEOMETRY      11000000000000000.0f
#define LOD_VIEW_COMMANDER_FIRE_GEOMETRY 12000000000000000.0f
#define LOD_VIEW_PILOT_GEOMETRY          13000000000000000.0f
#define LOD_VIEW_PILOT_FIRE_GEOMETRY     14000000000000000.0f
#define LOD_VIEW_GUNNER_GEOMETRY         15000000000000000.0f
#define LOD_VIEW_GUNNER_FIRE_GEOMETRY    16000000000000000.0f
#define LOD_SUB_PARTS                    17000000000000000.0f
#define LOD_SHADOW_VOLUME_VIEW_CARGO     18000000000000000.0f
#define LOD_SHADOW_VOLUME_VIEW_PILOT     19000000000000000.0f
#define LOD_SHADOW_VOLUME_VIEW_GUNNER    20000000000000000.0f
#define LOD_WRECK                        21000000000000000.0f


#include "utils.h"
#include "model_config.h"


struct uv_pair {
    float u;
    float v;
};

struct property {
    char name[64];
    char value[64];
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
    char section_names[512];
};

struct mlod_selection {
    char name[512];
    uint8_t *points;
    uint8_t *faces;
};

struct mlod_lod {
    uint32_t num_points;
    uint32_t num_facenormals;
    uint32_t num_faces;
    uint32_t num_sharp_edges;
    struct point *points;
    struct triplet *facenormals;
    struct mlod_face *faces;
    float *mass;
    uint32_t *sharp_edges;
    struct property properties[MAXPROPERTIES];
    float resolution;
    uint32_t num_selections;
    struct mlod_selection *selections;
};

struct odol_face {
    uint8_t face_type;
#ifdef VERSION70
    uint32_t table[4];
#else
    uint16_t table[4];
#endif
};

struct odol_proxy {
    char name[512];
    struct triplet transform_x;
    struct triplet transform_y;
    struct triplet transform_z;
    struct triplet transform_n;
    uint32_t proxy_id;
    uint32_t selection_index;
    int32_t bone_index;
    uint32_t section_index;
};

struct odol_bonelink {
    uint32_t num_links;
    uint32_t links[4];
};

struct odol_section {
    uint32_t face_start;
    uint32_t face_end;
    uint32_t face_index_start;
    uint32_t face_index_end;
    uint32_t material_index_start;
    uint32_t material_index_end;
    uint32_t common_point_flags;
    int16_t common_texture_index;
    uint32_t common_face_flags;
    int32_t material_index;
    uint32_t unknown_long_1;
    float unknown_resolution_1;
    float unknown_resolution_2;
    uint32_t unknown_long_2;
};

struct odol_selection {
    char name[512];
    uint32_t num_faces;
#ifdef VERSION70
    uint32_t *faces;
#else
    uint16_t *faces;
#endif
    uint32_t always_0;
    bool is_sectional;
    uint32_t num_sections;
    uint32_t *sections;
    uint32_t num_vertices;
#ifdef VERSION70
    uint32_t *vertices;
#else
    uint16_t *vertices;
#endif
    uint32_t num_vertex_weights;
    uint8_t *vertex_weights;
};

struct odol_frame {
};

struct odol_lod {
    uint32_t num_proxies;
    struct odol_proxy *proxies;
    uint32_t num_items;
    uint32_t *items;
    uint32_t num_bonelinks;
    struct odol_bonelink *bonelinks;
    uint32_t num_points;
    uint32_t num_points_mlod;
    float unknown_v52_float;
    float unknown_float_1;
    float unknown_float_2;
    struct triplet min_pos;
    struct triplet max_pos;
    struct triplet autocenter_pos;
    float sphere;
    uint32_t num_textures;
    char *textures;
    uint32_t num_materials;
    struct material *materials;
#ifdef VERSION70
    uint32_t *point_to_vertex;
    uint32_t *vertex_to_point;
    uint32_t *face_lookup;
    uint32_t *face_lookup_reverse;
#else
    uint16_t *point_to_vertex;
    uint16_t *vertex_to_point;
    uint16_t *face_lookup;
    uint16_t *face_lookup_reverse;
#endif
    uint32_t num_faces;
    uint32_t offset_sections;
    uint16_t always_0;
    struct odol_face *faces;
    uint32_t num_sections;
    struct odol_section *sections;
    uint32_t num_selections;
    struct odol_selection *selections;
    uint32_t num_properties;
    struct property properties[MAXPROPERTIES];
    uint32_t num_frames;
    struct odol_frame *frames;
    uint32_t icon_color;
    uint32_t selected_color;
    uint32_t unknown_residue;
    char unknown_byte;
    float uv_scale[4];
    struct uv_pair *uv_coords;
    struct triplet *points;
    struct triplet *normals;
};

struct model_info {
    float *lod_resolutions;
    uint32_t index;
    float mem_lod_sphere;
    float geo_lod_sphere;
    uint32_t point_flags[3];
    struct triplet aiming_center;
    uint32_t map_icon_color;
    uint32_t map_selected_color;
    float view_density;
    struct triplet bbox_min;
    struct triplet bbox_max;
    struct triplet bbox_visual_min;
    struct triplet bbox_visual_max;
    struct triplet bounding_center;
    struct triplet geometry_center;
    struct triplet centre_of_mass;
    struct triplet inv_inertia;
    bool autocenter;
    bool lock_autocenter;
    bool can_occlude;
    bool can_be_occluded;
    bool allow_animation;
    char unknown_flags[6];
    uint32_t unknown_long;
    struct skeleton *skeleton;
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
