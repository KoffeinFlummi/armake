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


#define MAXBONES 512
#define MAXSECTIONS 1024
#define MAXANIMS 1024

#define TYPE_ROTATION      0
#define TYPE_ROTATION_X    1
#define TYPE_ROTATION_Y    2
#define TYPE_ROTATION_Z    3
#define TYPE_TRANSLATION   4
#define TYPE_TRANSLATION_X 5
#define TYPE_TRANSLATION_Y 6
#define TYPE_TRANSLATION_Z 7
#define TYPE_DIRECT        8
#define TYPE_HIDE 	   9

#define SOURCE_CLAMP  0
#define SOURCE_MIRROR 1
#define SOURCE_LOOP   2


#include "vector.h"

struct bone {
    char name[512];
    char parent[512];
};

struct animation {
    uint32_t type;
    char name[512];
    char selection[512];
    char source[512];
    char axis[512];
    char begin[512];
    char end[512];
    float min_value;
    float max_value;
    float min_phase;
    float max_phase;
    uint32_t junk;
    uint32_t always_0; // this might be centerfirstvertex? @todo
    uint32_t source_address;
    float angle0;
    float angle1;
    float offset0;
    float offset1;
    struct triplet axis_pos;
    struct triplet axis_dir;
    float angle;
    float axis_offset;
    float hide_value;
    float unhide_value;
};

struct skeleton {
    char name[512];
    uint32_t num_bones;
    struct bone bones[MAXBONES];
    uint32_t num_sections;
    char sections[MAXSECTIONS][512];
    uint32_t num_animations;
    struct animation animations[MAXANIMS];
    float ht_min;
    float ht_max;
    float af_max;
    float mf_max;
    float mf_act;
    float t_body;
};


int read_model_config(char *path, struct skeleton *skeleton);
