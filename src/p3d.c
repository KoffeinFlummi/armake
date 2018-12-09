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

#ifndef _WIN32
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "args.h"
#include "filesystem.h"
#include "utils.h"
#include "model_config.h"
#include "material.h"
#include "vector.h"
#include "matrix.h"
#include "p3d.h"


int read_lods(FILE *f_source, struct mlod_lod *mlod_lods, uint32_t num_lods) {
    /*
     * Reads all LODs (starting at the current position of f_source) into
     * the given LODs array.
     *
     * Returns number of read lods on success and a negative integer on
     * failure.
     */

    char buffer[256];
    int i;
    int j;
    int fp_tmp;
    int fp_taggs;
    bool empty;
    uint32_t tagg_len;

    fseek(f_source, 0, SEEK_SET);

    fread(buffer, 4, 1, f_source);
    buffer[4] = 0;
    if (stricmp(buffer, "MLOD"))
        return -1;

    fseek(f_source, 12, SEEK_SET);

    for (i = 0; i < num_lods; i++) {
        fread(buffer, 4, 1, f_source);
        if (strncmp(buffer, "P3DM", 4) != 0)
            return -1;

        fseek(f_source, 8, SEEK_CUR);
        fread(&mlod_lods[i].num_points, 4, 1, f_source);
        fread(&mlod_lods[i].num_facenormals, 4, 1, f_source);
        fread(&mlod_lods[i].num_faces, 4, 1, f_source);
        fseek(f_source, 4, SEEK_CUR);

        empty = mlod_lods[i].num_points == 0;

        if (empty) {
            mlod_lods[i].num_points = 1;
            mlod_lods[i].points = (struct point *)safe_malloc(sizeof(struct point));
            mlod_lods[i].points[0].x = 0.0f;
            mlod_lods[i].points[0].y = 0.0f;
            mlod_lods[i].points[0].z = 0.0f;
            mlod_lods[i].points[0].point_flags = 0;
        } else {
            mlod_lods[i].points = (struct point *)safe_malloc(sizeof(struct point) * mlod_lods[i].num_points);
            for (j = 0; j < mlod_lods[i].num_points; j++)
                fread(&mlod_lods[i].points[j], sizeof(struct point), 1, f_source);
        }

        mlod_lods[i].facenormals = (struct triplet *)safe_malloc(sizeof(struct triplet) * mlod_lods[i].num_facenormals);
        for (j = 0; j < mlod_lods[i].num_facenormals; j++)
            fread(&mlod_lods[i].facenormals[j], sizeof(struct triplet), 1, f_source);

        mlod_lods[i].faces = (struct mlod_face *)safe_malloc(sizeof(struct mlod_face) * mlod_lods[i].num_faces);
        for (j = 0; j < mlod_lods[i].num_faces; j++) {
            fread(&mlod_lods[i].faces[j], 72, 1, f_source);

            fp_tmp = ftell(f_source);
            fread(mlod_lods[i].faces[j].texture_name,
                sizeof(mlod_lods[i].faces[j].texture_name), 1, f_source);
            fseek(f_source, fp_tmp + strlen(mlod_lods[i].faces[j].texture_name) + 1, SEEK_SET);

            fp_tmp = ftell(f_source);
            fread(mlod_lods[i].faces[j].material_name,
                sizeof(mlod_lods[i].faces[j].material_name), 1, f_source);
            fseek(f_source, fp_tmp + strlen(mlod_lods[i].faces[j].material_name) + 1, SEEK_SET);

            strcpy(mlod_lods[i].faces[j].section_names, "");
        }

        fread(buffer, 4, 1, f_source);
        if (strncmp(buffer, "TAGG", 4) != 0)
            return -2;

        mlod_lods[i].mass = 0;
        mlod_lods[i].num_sharp_edges = 0;
        mlod_lods[i].sharp_edges = 0;
        mlod_lods[i].selections = 0;

        for (j = 0; j < MAXPROPERTIES; j++) {
            mlod_lods[i].properties[j].name[0] = 0;
            mlod_lods[i].properties[j].value[0] = 0;
        }

        fp_taggs = ftell(f_source);

        // count selections
        mlod_lods[i].num_selections = 0;
        while (true) {
            fseek(f_source, 1, SEEK_CUR);

            fp_tmp = ftell(f_source);
            fread(&buffer, sizeof(buffer), 1, f_source);
            fseek(f_source, fp_tmp + strlen(buffer) + 1, SEEK_SET);

            fread(&tagg_len, 4, 1, f_source);
            fp_tmp = ftell(f_source) + tagg_len;

            if (buffer[0] != '#')
                mlod_lods[i].num_selections++;

            fseek(f_source, fp_tmp, SEEK_SET);

            if (strcmp(buffer, "#EndOfFile#") == 0)
                break;
        }

        mlod_lods[i].selections = (struct mlod_selection *)safe_malloc(sizeof(struct mlod_selection) * mlod_lods[i].num_selections);
        for (j = 0; j < mlod_lods[i].num_selections; j++)
            mlod_lods[i].selections[j].name[0] = 0;

        fseek(f_source, fp_taggs, SEEK_SET);

        while (true) {
            fseek(f_source, 1, SEEK_CUR);

            fp_tmp = ftell(f_source);
            fread(&buffer, sizeof(buffer), 1, f_source);
            fseek(f_source, fp_tmp + strlen(buffer) + 1, SEEK_SET);

            fread(&tagg_len, 4, 1, f_source);
            fp_tmp = ftell(f_source) + tagg_len;

            if (buffer[0] != '#') {
                for (j = 0; j < mlod_lods[i].num_selections; j++) {
                    if (mlod_lods[i].selections[j].name[0] == 0)
                        break;
                }

                strcpy(mlod_lods[i].selections[j].name, buffer);

                if (empty) {
                    mlod_lods[i].selections[j].points = (uint8_t *)safe_malloc(1);
                    mlod_lods[i].selections[j].points[0] = 0;
                } else {
                    mlod_lods[i].selections[j].points = (uint8_t *)safe_malloc(mlod_lods[i].num_points);
                    fread(mlod_lods[i].selections[j].points, mlod_lods[i].num_points, 1, f_source);
                }

                mlod_lods[i].selections[j].faces = (uint8_t *)safe_malloc(mlod_lods[i].num_faces);
                fread(mlod_lods[i].selections[j].faces, mlod_lods[i].num_faces, 1, f_source);
            }

            if (strcmp(buffer, "#Mass#") == 0) {
                if (empty) {
                    mlod_lods[i].mass = (float *)safe_malloc(sizeof(float));
                    mlod_lods[i].mass[0] = 0.0f;
                } else {
                    mlod_lods[i].mass = (float *)safe_malloc(sizeof(float) * mlod_lods[i].num_points);
                    fread(mlod_lods[i].mass, sizeof(float) * mlod_lods[i].num_points, 1, f_source);
                }
            }

            if (strcmp(buffer, "#SharpEdges#") == 0) {
                mlod_lods[i].num_sharp_edges = tagg_len / (2 * sizeof(uint32_t));
                mlod_lods[i].sharp_edges = (uint32_t *)safe_malloc(tagg_len);
                fread(mlod_lods[i].sharp_edges, tagg_len, 1, f_source);
            }

            if (strcmp(buffer, "#Property#") == 0) {
                for (j = 0; j < MAXPROPERTIES; j++) {
                    if (mlod_lods[i].properties[j].name[0] == 0)
                        break;
                }
                if (j == MAXPROPERTIES)
                    return -3;

                fread(mlod_lods[i].properties[j].name, 64, 1, f_source);
                fread(mlod_lods[i].properties[j].value, 64, 1, f_source);
            }

            fseek(f_source, fp_tmp, SEEK_SET);

            if (strcmp(buffer, "#EndOfFile#") == 0)
                break;
        }

        fread(&mlod_lods[i].resolution, 4, 1, f_source);

        if (mlod_lods[i].resolution >= LOD_EDIT_START && mlod_lods[i].resolution < LOD_EDIT_END) {
            free(mlod_lods[i].points);
            free(mlod_lods[i].facenormals);
            free(mlod_lods[i].faces);
            free(mlod_lods[i].mass);
            free(mlod_lods[i].sharp_edges);

            for (j = 0; j < mlod_lods[i].num_selections; j++) {
                free(mlod_lods[i].selections[j].points);
                free(mlod_lods[i].selections[j].faces);
            }

            free(mlod_lods[i].selections);

            i--;
            num_lods--;
        }
    }

    return num_lods;
}


void get_bounding_box(struct mlod_lod *mlod_lods, uint32_t num_lods,
        struct triplet *bbox_min, struct triplet *bbox_max, bool visual_only, bool geometry_only) {
    /*
     * Calculate the bounding box for the given LODs and stores it
     * in the given triplets.
     */

    bool first;
    float x;
    float y;
    float z;
    int i;
    int j;

    memset(bbox_min, 0, sizeof(struct triplet));
    memset(bbox_max, 0, sizeof(struct triplet));

    first = true;

    for (i = 0; i < num_lods; i++) {
        if (mlod_lods[i].resolution > LOD_GRAPHICAL_END && visual_only)
            continue;

        if (!float_equal(mlod_lods[i].resolution, LOD_GEOMETRY, 0.01) && geometry_only)
            continue;

        for (j = 0; j < mlod_lods[i].num_points; j++) {
            x = mlod_lods[i].points[j].x;
            y = mlod_lods[i].points[j].y;
            z = mlod_lods[i].points[j].z;

            if (first || x < bbox_min->x)
                bbox_min->x = x;
            if (first || x > bbox_max->x)
                bbox_max->x = x;

            if (first || y < bbox_min->y)
                bbox_min->y = y;
            if (first || y > bbox_max->y)
                bbox_max->y = y;

            if (first || z < bbox_min->z)
                bbox_min->z = z;
            if (first || z > bbox_max->z)
                bbox_max->z = z;

            first = false;
        }
    }
}


float get_sphere(struct mlod_lod *mlod_lod, vector center) {
    /*
     * Calculate and return the bounding sphere for the given LOD.
     */

    long i;
    float sphere;
    float dist;
    vector point;

    sphere = 0;
    for (i = 0; i < mlod_lod->num_points; i++) {
        point = *((vector*)&mlod_lod->points[i].x);
        dist = vector_length(vector_sub(point, center));
        if (dist > sphere)
            sphere = dist;
    }

    return sphere;
}


void get_mass_data(struct mlod_lod *mlod_lods, uint32_t num_lods, struct model_info *model_info) {
    int i;
    float mass;
    vector sum;
    vector pos;
    matrix inertia;
    matrix r_tilda;
    struct mlod_lod *mass_lod;

    // mass is primarily stored in geometry
    for (i = 0; i < num_lods; i++) {
        if (float_equal(mlod_lods[i].resolution, LOD_GEOMETRY, 0.01))
            break;
    }

    // alternatively use the PhysX LOD
    if (i >= num_lods || mlod_lods[i].num_points == 0) {
        for (i = 0; i < num_lods; i++) {
            if (float_equal(mlod_lods[i].resolution, LOD_PHYSX, 0.01))
                break;
        }
    }

    // mass data available?
    if (i >= num_lods || mlod_lods[i].num_points == 0) {
        model_info->mass = 0;
        model_info->mass_reciprocal = 1;
        model_info->inv_inertia = identity_matrix;
        model_info->centre_of_mass = empty_vector;
        return;
    }

    mass_lod = &mlod_lods[i];
    sum = empty_vector;
    mass = 0;
    for (i = 0; i < mass_lod->num_points; i++) {
        pos = *((vector *)&mass_lod->points[i].x);
        mass += mass_lod->mass[i];
        sum = vector_add(sum, vector_mult_scalar(mass_lod->mass[i], pos));
    }

    model_info->centre_of_mass = (mass > 0) ? vector_mult_scalar(1 / mass, sum) : empty_vector;

    inertia = empty_matrix;
    for (i = 0; i < mass_lod->num_points; i++) {
        pos = *((vector *)&mass_lod->points[i].x);
        r_tilda = vector_tilda(vector_sub(pos, model_info->centre_of_mass));
        inertia = matrix_sub(inertia, matrix_mult_scalar(mass_lod->mass[i], matrix_mult(r_tilda, r_tilda)));
    }

    // apply calculations to modelinfo
    model_info->mass = mass;
    if (mass > 0) {
        model_info->mass_reciprocal = 1 / mass;
        //model_info->inv_inertia = matrix_inverse(inertia);
        model_info->inv_inertia = empty_matrix;
        model_info->inv_inertia.m00 = 1.0f / inertia.m00;
        model_info->inv_inertia.m11 = 1.0f / inertia.m11;
        model_info->inv_inertia.m22 = 1.0f / inertia.m22;
    } else {
        model_info->mass_reciprocal = 1;
        model_info->inv_inertia = identity_matrix;
    }
}


void build_model_info(struct mlod_lod *mlod_lods, uint32_t num_lods, struct model_info *model_info) {
    int i;
    int j;
    float sphere;
    struct triplet bbox_total_min;
    struct triplet bbox_total_max;

    model_info->lod_resolutions = (float *)safe_malloc(sizeof(float) * num_lods);

    for (i = 0; i < num_lods; i++)
        model_info->lod_resolutions[i] = mlod_lods[i].resolution;

    model_info->index = 0;

    model_info->autocenter = false;
    model_info->lock_autocenter = false; // @todo
    model_info->can_occlude = false; // @todo
    model_info->can_be_occluded = true; // @todo
    model_info->ai_cover = false; // @todo
    model_info->animated = false; // @todo

    for (i = 0; i < num_lods; i++) {
        for (j = 0; j < MAXPROPERTIES; j++) {
            if (strcmp(mlod_lods[i].properties[j].name, "autocenter") != 0)
                continue;
            if (strcmp(mlod_lods[i].properties[j].value, "1") == 0) {
                model_info->autocenter = true;
                break;
            }
        }
    }

    // Bounding box & center
    get_bounding_box(mlod_lods, num_lods, &bbox_total_min, &bbox_total_max, false, false);

    model_info->bounding_center = vector_mult_scalar(0.5f, vector_add(bbox_total_min, bbox_total_max));

    if (!model_info->autocenter)
        model_info->bounding_center = empty_vector;

    model_info->bbox_min = vector_sub(bbox_total_min, model_info->bounding_center);
    model_info->bbox_max = vector_sub(bbox_total_max, model_info->bounding_center);

    model_info->lod_density_coef = 1.0f; // @todo
    model_info->draw_importance = 1.0f; // @todo

    // Visual bounding box
    get_bounding_box(mlod_lods, num_lods, &model_info->bbox_visual_min, &model_info->bbox_visual_max, true, false);

    model_info->bbox_visual_min = vector_sub(model_info->bbox_visual_min, model_info->bounding_center);
    model_info->bbox_visual_max = vector_sub(model_info->bbox_visual_max, model_info->bounding_center);

    // Geometry center
    get_bounding_box(mlod_lods, num_lods, &bbox_total_min, &bbox_total_max, false, true);

    model_info->geometry_center = vector_sub(vector_mult_scalar(0.5, vector_add(bbox_total_min, bbox_total_max)), model_info->bounding_center);

    // Centre of mass, inverse inertia, mass and inverse mass
    get_mass_data(mlod_lods, num_lods, model_info);

    // Aiming Center
    // @todo: i think this uses the fire geo lod if available
    model_info->aiming_center = model_info->geometry_center;

    // Spheres
    model_info->bounding_sphere = 0.0f;
    model_info->geo_lod_sphere = 0.0f;
    for (i = 0; i < num_lods; i++) {
        if (model_info->autocenter)
            sphere = get_sphere(&mlod_lods[i], model_info->centre_of_mass);
        else
            sphere = get_sphere(&mlod_lods[i], empty_vector);

        if (sphere > model_info->bounding_sphere)
            model_info->bounding_sphere = sphere;
        if (float_equal(mlod_lods[i].resolution, LOD_GEOMETRY, 0.01))
            model_info->geo_lod_sphere = sphere;
    }

    memset(model_info->point_flags, 0, sizeof(model_info->point_flags));

    model_info->map_icon_color = 0xff9d8254;
    model_info->map_selected_color = 0xff9d8254;

    model_info->view_density = -100.0f; // @todo

    model_info->force_not_alpha = false; //@todo
    model_info->sb_source = 0; //@todo
    model_info->prefer_shadow_volume = false; //@todo
    model_info->shadow_offset = 1.0f; //@todo

    model_info->skeleton = (struct skeleton *)safe_malloc(sizeof(struct skeleton));
    memset(model_info->skeleton, 0, sizeof(struct skeleton));

    model_info->map_type = 22; //@todo
    model_info->n_floats = 0;

    model_info->armor = 200.0f; // @todo
    model_info->inv_armor = 0.005f; // @todo

    // the real indices to the lods are calculated by the engine when the model is loaded
    memset(&model_info->special_lod_indices, 255, sizeof(struct lod_indices));
    for (i = 0; i < num_lods; i++) {
        if (mlod_lods[i].resolution == LOD_MEMORY)
            model_info->special_lod_indices.memory = i;
        if (mlod_lods[i].resolution == LOD_GEOMETRY)
            model_info->special_lod_indices.geometry = i;
        if (mlod_lods[i].resolution == LOD_PHYSX)
            model_info->special_lod_indices.geometry_physx = i;
        if (mlod_lods[i].resolution == LOD_FIRE_GEOMETRY)
            model_info->special_lod_indices.geometry_fire = i;
        if (mlod_lods[i].resolution == LOD_VIEW_GEOMETRY)
            model_info->special_lod_indices.geometry_view = i;
        if (mlod_lods[i].resolution == LOD_VIEW_PILOT_GEOMETRY)
            model_info->special_lod_indices.geometry_view_pilot = i;
        if (mlod_lods[i].resolution == LOD_VIEW_GUNNER_GEOMETRY)
            model_info->special_lod_indices.geometry_view_gunner = i;
        if (mlod_lods[i].resolution == LOD_VIEW_CARGO_GEOMETRY)
            model_info->special_lod_indices.geometry_view_cargo = i;
        if (mlod_lods[i].resolution == LOD_LAND_CONTACT)
            model_info->special_lod_indices.land_contact = i;
        if (mlod_lods[i].resolution == LOD_ROADWAY)
            model_info->special_lod_indices.roadway = i;
        if (mlod_lods[i].resolution == LOD_PATHS)
            model_info->special_lod_indices.paths = i;
        if (mlod_lods[i].resolution == LOD_HITPOINTS)
            model_info->special_lod_indices.hitpoints = i;
    }

    if (model_info->special_lod_indices.geometry_view == -1)
        model_info->special_lod_indices.geometry_view = model_info->special_lod_indices.geometry;

    if (model_info->special_lod_indices.geometry_fire == -1)
        model_info->special_lod_indices.geometry_fire = model_info->special_lod_indices.geometry;

    // according to BIS: first LOD that can be used for shadowing
    model_info->min_shadow = num_lods; // @todo
    model_info->can_blend = false; // @todo
    model_info->class_type = 0; // @todo
    model_info->destruct_type = 0; // @todo
    model_info->property_frequent = false; //@todo
    model_info->always_0 = 0;
}


uint32_t add_point(struct odol_lod *odol_lod, struct mlod_lod *mlod_lod, struct model_info *model_info,
        uint32_t point_index_mlod, struct triplet *normal, struct uv_pair *uv_coords) {
    uint32_t i;
    uint32_t j;
    uint32_t weight_index;

    // Check if there already is a vertex that satisfies the requirements
    for (i = 0; i < odol_lod->num_points; i++) {
        if (odol_lod->vertex_to_point[i] != point_index_mlod)
            continue;

        // normals and uvs don't matter for non-visual lods
        if (mlod_lod->resolution < LOD_GEOMETRY) {
            if (!float_equal(odol_lod->normals[i].x, normal->x, 0.0001) ||
                    !float_equal(odol_lod->normals[i].y, normal->y, 0.0001) ||
                    !float_equal(odol_lod->normals[i].z, normal->z, 0.0001))
                continue;

            if (!float_equal(odol_lod->uv_coords[i].u, uv_coords->u, 0.0001) ||
                    !float_equal(odol_lod->uv_coords[i].v, uv_coords->v, 0.0001))
                continue;
        }

        return i;
    }

    // Add vertex
    memcpy(&odol_lod->points[odol_lod->num_points], &mlod_lod->points[point_index_mlod], sizeof(struct triplet));
    memcpy(&odol_lod->normals[odol_lod->num_points], normal, sizeof(struct triplet));
    memcpy(&odol_lod->uv_coords[odol_lod->num_points], uv_coords, sizeof(struct uv_pair));

    if (odol_lod->vertexboneref != 0 && model_info->skeleton->num_bones > 0) {
        memset(&odol_lod->vertexboneref[odol_lod->num_points], 0, sizeof(struct odol_vertexboneref));

        for (i = model_info->skeleton->num_bones - 1; (int32_t)i >= 0; i--) {
            for (j = 0; j < mlod_lod->num_selections; j++) {
                if (stricmp(model_info->skeleton->bones[i].name,
                        mlod_lod->selections[j].name) == 0)
                    break;
            }

            if (j == mlod_lod->num_selections)
                continue;

            if (mlod_lod->selections[j].points[point_index_mlod] == 0)
                continue;

            if (odol_lod->vertexboneref[odol_lod->num_points].num_bones == 4) {
                lwarningf(current_target, -1, "Vertex %u of LOD %f is part of more than 4 bones.\n", point_index_mlod, mlod_lod->resolution);
                continue;
            }

            if (odol_lod->vertexboneref[odol_lod->num_points].num_bones == 1 && model_info->skeleton->is_discrete) {
                lwarningf(current_target, -1, "Vertex %u of LOD %f is part of more than 1 bone in a discrete skeleton.\n", point_index_mlod, mlod_lod->resolution);
                continue;
            }

            weight_index = odol_lod->vertexboneref[odol_lod->num_points].num_bones;
            odol_lod->vertexboneref[odol_lod->num_points].num_bones++;

            odol_lod->vertexboneref[odol_lod->num_points].weights[weight_index][0] = odol_lod->skeleton_to_subskeleton[i].links[0];
            odol_lod->vertexboneref[odol_lod->num_points].weights[weight_index][1] = mlod_lod->selections[j].points[point_index_mlod];

            // convert weight
            if (odol_lod->vertexboneref[odol_lod->num_points].weights[weight_index][1] == 0x01)
                odol_lod->vertexboneref[odol_lod->num_points].weights[weight_index][1] = 0xff;
            else
                odol_lod->vertexboneref[odol_lod->num_points].weights[weight_index][1]--;

            if (model_info->skeleton->is_discrete)
                break;
        }
    }

    odol_lod->vertex_to_point[odol_lod->num_points] = point_index_mlod;
    odol_lod->point_to_vertex[point_index_mlod] = odol_lod->num_points;

    odol_lod->num_points++;

    return (odol_lod->num_points - 1);
}


int flags_to_pass(uint32_t flags) {
    if (flags & FLAG_DSTBLENDZERO)
        return 0;
    if (flags & FLAG_NOALPHAWRITE)
        return 2;
    if (flags & FLAG_NOCOLORWRITE)
        return 3;
    if (flags & FLAG_NOZWRITE)
        return 4;
    return 1;
}


#ifdef _WIN32
int compare_face_lookup(void *faces_ptr, const void *a, const void *b) {
#else
int compare_face_lookup(const void *a, const void *b, void *faces_ptr) {
#endif
    struct mlod_face *faces;
    uint32_t a_index;
    uint32_t b_index;
    int compare;

    faces = (struct mlod_face *)faces_ptr;
    a_index = *((uint32_t *)a);
    b_index = *((uint32_t *)b);

    compare = faces[a_index].material_index - faces[b_index].material_index;
    if (compare != 0)
        return compare;

    compare = faces[a_index].face_flags - faces[b_index].face_flags;
    if (compare != 0)
        return compare;

    compare = faces[a_index].texture_index - faces[b_index].texture_index;
    if (compare != 0)
        return compare;

    return strcmp(faces[a_index].section_names, faces[b_index].section_names);
}


bool is_alpha(struct mlod_face *face) {
    // @todo check actual texture maybe?
    if (strstr(face->texture_name, "_ca.paa") != NULL)
        return true;
    if (strstr(face->texture_name, "ca)") != NULL)
        return true;
    return false;
}


void convert_lod(struct mlod_lod *mlod_lod, struct odol_lod *odol_lod,
        struct model_info *model_info) {
    extern char *current_target;
    unsigned long i;
    unsigned long j;
    unsigned long k;
    unsigned long face;
    unsigned long face_start;
    unsigned long face_end;
    size_t size;
    char *ptr;
    char textures[MAXTEXTURES][512];
    char *temp;
    bool *tileU;
    bool *tileV;
    struct triplet normal;
    struct uv_pair uv_coords;

    // Set sub skeleton references
    odol_lod->num_bones_skeleton = model_info->skeleton->num_bones;
    odol_lod->num_bones_subskeleton = model_info->skeleton->num_bones;
    odol_lod->subskeleton_to_skeleton = (uint32_t *)safe_malloc(sizeof(uint32_t) * odol_lod->num_bones_skeleton);
    odol_lod->skeleton_to_subskeleton = (struct odol_bonelink *)safe_malloc(sizeof(struct odol_bonelink) * odol_lod->num_bones_skeleton);

    for (i = 0; i < model_info->skeleton->num_bones; i++) {
        odol_lod->subskeleton_to_skeleton[i] = i;
        odol_lod->skeleton_to_subskeleton[i].num_links = 1;
        odol_lod->skeleton_to_subskeleton[i].links[0] = i;
    }

    odol_lod->num_points_mlod = mlod_lod->num_points;

    odol_lod->face_area = 0;
    odol_lod->clip_flags[0] = 0;
    odol_lod->clip_flags[1] = 0;

    odol_lod->min_pos = empty_vector;
    odol_lod->max_pos = empty_vector;

    for (i = 0; i < mlod_lod->num_points; i++) {
        if (i == 0 || mlod_lod->points[i].x < odol_lod->min_pos.x)
            odol_lod->min_pos.x = mlod_lod->points[i].x;
        if (i == 0 || mlod_lod->points[i].y < odol_lod->min_pos.y)
            odol_lod->min_pos.y = mlod_lod->points[i].y;
        if (i == 0 || mlod_lod->points[i].z < odol_lod->min_pos.z)
            odol_lod->min_pos.z = mlod_lod->points[i].z;
        if (i == 0 || mlod_lod->points[i].x > odol_lod->max_pos.x)
            odol_lod->max_pos.x = mlod_lod->points[i].x;
        if (i == 0 || mlod_lod->points[i].y > odol_lod->max_pos.y)
            odol_lod->max_pos.y = mlod_lod->points[i].y;
        if (i == 0 || mlod_lod->points[i].z > odol_lod->max_pos.z)
            odol_lod->max_pos.z = mlod_lod->points[i].z;
    }

    odol_lod->autocenter_pos = vector_mult_scalar(0.5, vector_add(odol_lod->min_pos, odol_lod->max_pos));

    odol_lod->sphere = get_sphere(mlod_lod, odol_lod->autocenter_pos);

    // Textures & Materials
    odol_lod->num_textures = 0;
    odol_lod->num_materials = 0;
    odol_lod->materials = (struct material *)safe_malloc(sizeof(struct material) * MAXMATERIALS);
    memset(textures, 0, sizeof(textures));
    memset(odol_lod->materials, 0, sizeof(struct material) * MAXMATERIALS);

    size = 0;
    for (i = 0; i < mlod_lod->num_faces; i++) {
        for (j = 0; j < odol_lod->num_textures; j++) {
            if (strcmp(mlod_lod->faces[i].texture_name, textures[j]) == 0)
                break;
        }

        mlod_lod->faces[i].texture_index = j;

        if (j >= MAXTEXTURES) {
            lwarningf(current_target, -1, "Maximum amount of textures per LOD (%i) exceeded.", MAXTEXTURES);
            break;
        }

        if (j >= odol_lod->num_textures) {
            strcpy(textures[j], mlod_lod->faces[i].texture_name);
            size += strlen(textures[j]) + 1;
            odol_lod->num_textures++;
        }

        for (j = 0; j < MAXMATERIALS && odol_lod->materials[j].path[0] != 0; j++) {
            if (strcmp(mlod_lod->faces[i].material_name, odol_lod->materials[j].path) == 0)
                break;
        }

        mlod_lod->faces[i].material_index = (strlen(mlod_lod->faces[i].material_name) > 0) ? j : -1;

        if (j >= MAXMATERIALS) {
            lwarningf(current_target, -1, "Maximum amount of materials per LOD (%i) exceeded.", MAXMATERIALS);
            break;
        }

        if (odol_lod->materials[j].path[0] != 0 || mlod_lod->faces[i].material_name[0] == 0)
            continue;

        temp = current_target;

        strcpy(odol_lod->materials[j].path, mlod_lod->faces[i].material_name);
        odol_lod->num_materials++;
        read_material(&odol_lod->materials[j]);

        current_target = temp;
    }

    odol_lod->textures = (char *)safe_malloc(size);
    ptr = odol_lod->textures;
    for (i = 0; i < odol_lod->num_textures; i++) {
        strncpy(ptr, textures[i], strlen(textures[i]) + 1);
        ptr += strlen(textures[i]) + 1;
    }

    odol_lod->num_faces = mlod_lod->num_faces;

    odol_lod->always_0 = 0;

    odol_lod->faces = (struct odol_face *)safe_malloc(sizeof(struct odol_face) * odol_lod->num_faces);
    memset(odol_lod->faces, 0, sizeof(struct odol_face) * odol_lod->num_faces);

    odol_lod->num_points = 0;

    odol_lod->point_to_vertex = (uint32_t *)safe_malloc(sizeof(uint32_t) * odol_lod->num_points_mlod);
    odol_lod->vertex_to_point = (uint32_t *)safe_malloc(sizeof(uint32_t) * (odol_lod->num_faces * 4 + odol_lod->num_points_mlod));
    odol_lod->face_lookup = (uint32_t *)safe_malloc(sizeof(uint32_t) * mlod_lod->num_faces);

    for (i = 0; i < mlod_lod->num_faces; i++)
        odol_lod->face_lookup[i] = i;

    for (i = 0; i < odol_lod->num_points_mlod; i++)
        odol_lod->point_to_vertex[i] = NOPOINT;

    odol_lod->uv_coords = (struct uv_pair *)safe_malloc(sizeof(struct uv_pair) * (odol_lod->num_faces * 4 + odol_lod->num_points_mlod));
    odol_lod->points = (struct triplet *)safe_malloc(sizeof(struct triplet) * (odol_lod->num_faces * 4 + odol_lod->num_points_mlod));
    odol_lod->normals = (struct triplet *)safe_malloc(sizeof(struct triplet) * (odol_lod->num_faces * 4 + odol_lod->num_points_mlod));

    odol_lod->vertexboneref = 0;
    if (model_info->skeleton->num_bones > 0)
        odol_lod->vertexboneref = (struct odol_vertexboneref *)safe_malloc(sizeof(struct odol_vertexboneref) * (odol_lod->num_faces * 4 + odol_lod->num_points_mlod));

    // Set face flags
    tileU = (bool *)safe_malloc(odol_lod->num_textures);
    tileV = (bool *)safe_malloc(odol_lod->num_textures);
    memset(tileU, 0, odol_lod->num_textures);
    memset(tileV, 0, odol_lod->num_textures);
    for (i = 0; i < mlod_lod->num_faces; i++) {
        if (strlen(mlod_lod->faces[i].texture_name) == 0)
            continue;
        if (tileU[mlod_lod->faces[i].texture_index] && tileV[mlod_lod->faces[i].texture_index])
            continue;
        for (j = 0; j < mlod_lod->faces[i].face_type; j++) {
            if (mlod_lod->faces[i].table[j].u < -CLAMPLIMIT || mlod_lod->faces[i].table[j].u > 1 + CLAMPLIMIT)
                tileU[mlod_lod->faces[i].texture_index] = 1;
            if (mlod_lod->faces[i].table[j].v < -CLAMPLIMIT || mlod_lod->faces[i].table[j].v > 1 + CLAMPLIMIT)
                tileV[mlod_lod->faces[i].texture_index] = 1;
        }
    }
    for (i = 0; i < mlod_lod->num_faces; i++) {
        if (mlod_lod->faces[i].face_flags & (FLAG_NOCLAMP | FLAG_CLAMPU | FLAG_CLAMPV))
            continue;
        if (strlen(mlod_lod->faces[i].texture_name) == 0) {
            mlod_lod->faces[i].face_flags |= FLAG_NOCLAMP;
            continue;
        }

        if (!tileU[mlod_lod->faces[i].texture_index])
            mlod_lod->faces[i].face_flags |= FLAG_CLAMPU;
        if (!tileV[mlod_lod->faces[i].texture_index])
            mlod_lod->faces[i].face_flags |= FLAG_CLAMPV;
        if (tileU[mlod_lod->faces[i].texture_index] && tileU[mlod_lod->faces[i].texture_index])
            mlod_lod->faces[i].face_flags |= FLAG_NOCLAMP;

        if (is_alpha(&mlod_lod->faces[i]))
            mlod_lod->faces[i].face_flags |= FLAG_ISALPHA;
    }
    free(tileU);
    free(tileV);

    for (i = 0; i < mlod_lod->num_selections; i++) {
        for (j = 0; j < model_info->skeleton->num_sections; j++) {
            if (strcmp(mlod_lod->selections[i].name, model_info->skeleton->sections[j]) == 0)
                break;
        }
        if (j < model_info->skeleton->num_sections) {
            for (k = 0; k < mlod_lod->num_faces; k++) {
                if (mlod_lod->selections[i].faces[k] > 0) {
                    strcat(mlod_lod->faces[k].section_names, ":");
                    strcat(mlod_lod->faces[k].section_names, mlod_lod->selections[i].name);
                }
            }
        }

        if (strncmp(mlod_lod->selections[i].name, "proxy:", 6) != 0)
            continue;

        for (k = 0; k < mlod_lod->num_faces; k++) {
            if (mlod_lod->selections[i].faces[k] > 0) {
                mlod_lod->faces[k].face_flags |= FLAG_ISHIDDENPROXY;
                mlod_lod->faces[k].texture_index = -1;
                mlod_lod->faces[k].material_index = -1;
            }
        }
    }

    // Sort faces
    if (mlod_lod->num_faces > 1) {
#ifdef _WIN32
        qsort_s(odol_lod->face_lookup, odol_lod->num_faces, sizeof(uint32_t), compare_face_lookup, (void *)mlod_lod->faces);
#else
        qsort_r(odol_lod->face_lookup, odol_lod->num_faces, sizeof(uint32_t), compare_face_lookup, (void *)mlod_lod->faces);
#endif
    }

    // Write face vertices
    face_end = 0;
    memset(odol_lod->uv_scale, 0, sizeof(struct uv_pair) * 2);
    for (i = 0; i < mlod_lod->num_faces; i++) {
        odol_lod->faces[i].face_type = mlod_lod->faces[odol_lod->face_lookup[i]].face_type;
        for (j = 0; j < odol_lod->faces[i].face_type; j++) {
            memcpy(&normal, &mlod_lod->facenormals[mlod_lod->faces[odol_lod->face_lookup[i]].table[j].normals_index], sizeof(struct triplet));
            uv_coords.u = mlod_lod->faces[odol_lod->face_lookup[i]].table[j].u;
            uv_coords.v = mlod_lod->faces[odol_lod->face_lookup[i]].table[j].v;

            uv_coords.u = fsign(uv_coords.u) * (fmod(fabs(uv_coords.u), 1.0));
            uv_coords.v = fsign(uv_coords.v) * (fmod(fabs(uv_coords.v), 1.0));

            odol_lod->uv_scale[0].u = fminf(uv_coords.u, odol_lod->uv_scale[0].u);
            odol_lod->uv_scale[0].v = fminf(uv_coords.v, odol_lod->uv_scale[0].v);
            odol_lod->uv_scale[1].u = fmaxf(uv_coords.u, odol_lod->uv_scale[1].u);
            odol_lod->uv_scale[1].v = fmaxf(uv_coords.v, odol_lod->uv_scale[1].v);

            // Change vertex order for ODOL
            // Tris:  0 1 2   -> 1 0 2
            // Quads: 0 1 2 3 -> 1 0 3 2
            if (odol_lod->faces[i].face_type == 4)
                k = j ^ 1;
            else
                k = j ^ (1 ^ (j >> 1));

            odol_lod->faces[i].table[k] = add_point(odol_lod, mlod_lod, model_info,
                mlod_lod->faces[odol_lod->face_lookup[i]].table[j].points_index, &normal, &uv_coords);
        }
        face_end += (odol_lod->faces[i].face_type == 4) ? 20 : 16;
    }

    odol_lod->face_allocation_size = face_end;

    // Write remaining vertices
    for (i = 0; i < odol_lod->num_points_mlod; i++) {
        if (odol_lod->point_to_vertex[i] < NOPOINT)
            continue;

        normal = empty_vector;

        uv_coords.u = 0.0f;
        uv_coords.v = 0.0f;

        odol_lod->point_to_vertex[i] = add_point(odol_lod, mlod_lod, model_info,
            i, &normal, &uv_coords);
    }

    // Normalize vertex bone ref
    odol_lod->vertexboneref_is_simple = 1;
    float weight_sum;
    if (odol_lod->vertexboneref != 0) {
        for (i = 0; i < odol_lod->num_points; i++) {
            if (odol_lod->vertexboneref[i].num_bones == 0)
                continue;

            if (odol_lod->vertexboneref[i].num_bones > 1)
                odol_lod->vertexboneref_is_simple = 0;

            weight_sum = 0;
            for (j = 0; j < odol_lod->vertexboneref[i].num_bones; j++) {
                weight_sum += odol_lod->vertexboneref[i].weights[j][1] / 255.0f;
            }

            for (j = 0; j < odol_lod->vertexboneref[i].num_bones; j++) {
                odol_lod->vertexboneref[i].weights[j][1] *= (1.0 / weight_sum);
            }
        }
    }

    // Sections
    if (odol_lod->num_faces > 0) {
        odol_lod->num_sections = 1;
        for (i = 1; i < odol_lod->num_faces; i++) {
#ifdef _WIN32
            if (compare_face_lookup((void *)mlod_lod->faces, &odol_lod->face_lookup[i], &odol_lod->face_lookup[i - 1])) {
#else
            if (compare_face_lookup(&odol_lod->face_lookup[i], &odol_lod->face_lookup[i - 1], (void *)mlod_lod->faces)) {
#endif
                odol_lod->num_sections++;
                continue;
            }
        }

        odol_lod->sections = (struct odol_section *)safe_malloc(sizeof(struct odol_section) * odol_lod->num_sections);

        face_start = 0;
        face_end = 0;
        k = 0;
        for (i = 0; i < odol_lod->num_faces;) {
            odol_lod->sections[k].face_start = i;
            odol_lod->sections[k].face_end = i;
            odol_lod->sections[k].face_index_start = face_start;
            odol_lod->sections[k].face_index_end = face_start;
            odol_lod->sections[k].min_bone_index = 0;
            odol_lod->sections[k].bones_count = odol_lod->num_bones_subskeleton;
            odol_lod->sections[k].mat_dummy = 0;
            odol_lod->sections[k].common_texture_index = mlod_lod->faces[odol_lod->face_lookup[i]].texture_index;
            odol_lod->sections[k].common_face_flags = mlod_lod->faces[odol_lod->face_lookup[i]].face_flags;
            odol_lod->sections[k].material_index = mlod_lod->faces[odol_lod->face_lookup[i]].material_index;
            odol_lod->sections[k].num_stages = 2; // num_stages defines number of entries in area_over_tex
            odol_lod->sections[k].area_over_tex[0] = 1.0f; // @todo
            odol_lod->sections[k].area_over_tex[1] = -1000.0f;
            odol_lod->sections[k].unknown_long = 0;

            for (j = i; j < odol_lod->num_faces; j++) {
#ifdef _WIN32
                if (compare_face_lookup((void *)mlod_lod->faces, &odol_lod->face_lookup[j], &odol_lod->face_lookup[i]))
#else
                if (compare_face_lookup(&odol_lod->face_lookup[j], &odol_lod->face_lookup[i], (void *)mlod_lod->faces))
#endif
                    break;

                odol_lod->sections[k].face_end++;
                odol_lod->sections[k].face_index_end += (odol_lod->faces[j].face_type == 4) ? 20 : 16;
            }

            face_start = odol_lod->sections[k].face_index_end;
            i = j;
            k++;
        }
    } else {
        odol_lod->num_sections = 0;
        odol_lod->sections = 0;
    }

    // Selections
    odol_lod->num_selections = mlod_lod->num_selections;
    odol_lod->selections = (struct odol_selection *)safe_malloc(sizeof(struct odol_selection) * odol_lod->num_selections);
    for (i = 0; i < odol_lod->num_selections; i++) {
        strcpy(odol_lod->selections[i].name, mlod_lod->selections[i].name);
        lower_case(odol_lod->selections[i].name);

        for (j = 0; j < model_info->skeleton->num_sections; j++) {
            if (strcmp(model_info->skeleton->sections[j], mlod_lod->selections[i].name) == 0)
                break;
        }
        odol_lod->selections[i].is_sectional = j < model_info->skeleton->num_sections;

        if (odol_lod->selections[i].is_sectional) {
            odol_lod->selections[i].num_faces = 0;
            odol_lod->selections[i].faces = 0;
            odol_lod->selections[i].always_0 = 0;
            odol_lod->selections[i].num_vertices = 0;
            odol_lod->selections[i].vertices = 0;
            odol_lod->selections[i].num_vertex_weights = 0;
            odol_lod->selections[i].vertex_weights = 0;

            odol_lod->selections[i].num_sections = 0;
            for (j = 0; j < odol_lod->num_sections; j++) {
                if (mlod_lod->selections[i].faces[odol_lod->face_lookup[odol_lod->sections[j].face_start]] > 0)
                    odol_lod->selections[i].num_sections++;
            }
            odol_lod->selections[i].sections = (uint32_t *)safe_malloc(sizeof(uint32_t) * odol_lod->selections[i].num_sections);
            k = 0;
            for (j = 0; j < odol_lod->num_sections; j++) {
                if (mlod_lod->selections[i].faces[odol_lod->face_lookup[odol_lod->sections[j].face_start]] > 0) {
                    odol_lod->selections[i].sections[k] = j;
                    k++;
                }
            }

            continue;
        } else {
            odol_lod->selections[i].num_sections = 0;
            odol_lod->selections[i].sections = 0;
        }

        odol_lod->selections[i].num_faces = 0;
        for (j = 0; j < odol_lod->num_faces; j++) {
            if (mlod_lod->selections[i].faces[j] > 0)
                odol_lod->selections[i].num_faces++;
        }

        odol_lod->selections[i].faces = (uint32_t *)safe_malloc(sizeof(uint32_t) * odol_lod->selections[i].num_faces);
        for (j = 0; j < odol_lod->selections[i].num_faces; j++)
            odol_lod->selections[i].faces[j] = NOPOINT;

        for (j = 0; j < odol_lod->num_faces; j++) {
            if (mlod_lod->selections[i].faces[j] == 0)
                continue;

            for (k = 0; k < odol_lod->selections[i].num_faces; k++) {
                if (odol_lod->selections[i].faces[k] == NOPOINT)
                    break;
            }
            odol_lod->selections[i].faces[k] = odol_lod->face_lookup[j];
        }

        odol_lod->selections[i].always_0 = 0;

        odol_lod->selections[i].num_vertices = 0;
        for (j = 0; j < odol_lod->num_points; j++) {
            if (mlod_lod->selections[i].points[odol_lod->vertex_to_point[j]] > 0)
                odol_lod->selections[i].num_vertices++;
        }

        odol_lod->selections[i].num_vertex_weights = odol_lod->selections[i].num_vertices;

        odol_lod->selections[i].vertices = (uint32_t *)safe_malloc(sizeof(uint32_t) * odol_lod->selections[i].num_vertices);
        for (j = 0; j < odol_lod->selections[i].num_vertices; j++)
            odol_lod->selections[i].vertices[j] = NOPOINT;

        odol_lod->selections[i].vertex_weights = (uint8_t *)safe_malloc(sizeof(uint8_t) * odol_lod->selections[i].num_vertex_weights);

        for (j = 0; j < odol_lod->num_points; j++) {
            if (mlod_lod->selections[i].points[odol_lod->vertex_to_point[j]] == 0)
                continue;

            for (k = 0; k < odol_lod->selections[i].num_vertices; k++) {
                if (odol_lod->selections[i].vertices[k] == NOPOINT)
                    break;
            }
            odol_lod->selections[i].vertices[k] = j;
            odol_lod->selections[i].vertex_weights[k] = mlod_lod->selections[i].points[odol_lod->vertex_to_point[j]];
        }
    }

    // Proxies
    odol_lod->num_proxies = 0;
    for (i = 0; i < mlod_lod->num_selections; i++) {
        if (strncmp(mlod_lod->selections[i].name, "proxy:", 6) == 0)
            odol_lod->num_proxies++;
    }
    odol_lod->proxies = (struct odol_proxy *)safe_malloc(sizeof(struct odol_proxy) * odol_lod->num_proxies);
    k = 0;
    for (i = 0; i < mlod_lod->num_selections; i++) {
        if (strncmp(mlod_lod->selections[i].name, "proxy:", 6) != 0)
            continue;

        for (j = 0; j < odol_lod->num_faces; j++) {
            if (mlod_lod->selections[i].faces[odol_lod->face_lookup[j]] > 0) {
                face = j;
                break;
            }
        }

        if (j >= mlod_lod->num_faces) {
            lnwarningf(current_target, -1, "no-proxy-face", "No face found for proxy \"%s\".\n", mlod_lod->selections[i].name + 6);
            odol_lod->num_proxies--;
            continue;
        }

        strcpy(odol_lod->proxies[k].name, mlod_lod->selections[i].name + 6);
        odol_lod->proxies[k].proxy_id = strtol(strrchr(odol_lod->proxies[k].name, '.') + 1, &ptr, 10);
        *strrchr(odol_lod->proxies[k].name, '.') = 0;
        lower_case(odol_lod->proxies[k].name);

        odol_lod->proxies[k].selection_index = i;
        odol_lod->proxies[k].bone_index = -1;

        if (odol_lod->vertexboneref != 0 &&
                odol_lod->vertexboneref[odol_lod->faces[face].table[0]].num_bones > 0) {
            odol_lod->proxies[k].bone_index = odol_lod->vertexboneref[odol_lod->faces[face].table[0]].weights[0][0];
        }

        for (j = 0; j < odol_lod->num_sections; j++) {
            if (face > odol_lod->sections[j].face_start)
                continue;
            odol_lod->proxies[k].section_index = j;
            break;
        }

        odol_lod->proxies[k].transform_y = vector_sub(
            *((vector *)&mlod_lod->points[mlod_lod->faces[odol_lod->face_lookup[face]].table[1].points_index]),
            *((vector *)&mlod_lod->points[mlod_lod->faces[odol_lod->face_lookup[face]].table[0].points_index]));
        odol_lod->proxies[k].transform_y = vector_normalize(odol_lod->proxies[k].transform_y);

        odol_lod->proxies[k].transform_z = vector_sub(
            *((vector *)&mlod_lod->points[mlod_lod->faces[odol_lod->face_lookup[face]].table[2].points_index]),
            *((vector *)&mlod_lod->points[mlod_lod->faces[odol_lod->face_lookup[face]].table[0].points_index]));
        odol_lod->proxies[k].transform_z = vector_normalize(odol_lod->proxies[k].transform_z);

        odol_lod->proxies[k].transform_x = vector_crossproduct(
            odol_lod->proxies[k].transform_y,
            odol_lod->proxies[k].transform_z);

        memcpy(&odol_lod->proxies[k].transform_n,
            &mlod_lod->points[mlod_lod->faces[odol_lod->face_lookup[face]].table[0].points_index],
            sizeof(struct triplet));

        k++;
    }

    // Properties
    odol_lod->num_properties = 0;
    for (i = 0; i < MAXPROPERTIES; i++) {
        if (mlod_lod->properties[i].name[0] != 0)
            odol_lod->num_properties++;
    }
    memcpy(odol_lod->properties, mlod_lod->properties, MAXPROPERTIES * sizeof(struct property));

    odol_lod->num_frames = 0; // @todo
    odol_lod->frames = 0;

    odol_lod->icon_color = 0xff9d8254;
    odol_lod->selected_color = 0xff9d8254;

    odol_lod->flags = 0;
}


void write_skeleton(FILE *f_target, struct skeleton *skeleton) {
    int i;

    fwrite(skeleton->name, strlen(skeleton->name) + 1, 1, f_target);

    if (strlen(skeleton->name) > 0) {
        fputc(0, f_target); // is inherited @todo ?
        fwrite(&skeleton->num_bones, sizeof(uint32_t), 1, f_target);
        for (i = 0; i < skeleton->num_bones; i++) {
            fwrite(skeleton->bones[i].name, strlen(skeleton->bones[i].name) + 1, 1, f_target);
            fwrite(skeleton->bones[i].parent, strlen(skeleton->bones[i].parent) + 1, 1, f_target);
        }
        fputc(0, f_target);
    }
}


void write_model_info(FILE *f_target, uint32_t num_lods, struct model_info *model_info) {
    int i;

    fwrite( model_info->lod_resolutions,     sizeof(float) * num_lods, 1, f_target);
    fwrite(&model_info->index,               sizeof(uint32_t), 1, f_target);
    fwrite(&model_info->bounding_sphere,     sizeof(float), 1, f_target);
    fwrite(&model_info->geo_lod_sphere,      sizeof(float), 1, f_target);
    fwrite( model_info->point_flags,         sizeof(uint32_t) * 3, 1, f_target);
    fwrite(&model_info->aiming_center,       sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->map_icon_color,      sizeof(uint32_t), 1, f_target);
    fwrite(&model_info->map_selected_color,  sizeof(uint32_t), 1, f_target);
    fwrite(&model_info->view_density,        sizeof(float), 1, f_target);
    fwrite(&model_info->bbox_min,            sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->bbox_max,            sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->lod_density_coef,    sizeof(float), 1, f_target);
    fwrite(&model_info->draw_importance,     sizeof(float), 1, f_target);
    fwrite(&model_info->bbox_visual_min,     sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->bbox_visual_max,     sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->bounding_center,     sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->geometry_center,     sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->centre_of_mass,      sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->inv_inertia,         sizeof(matrix), 1, f_target);
    fwrite(&model_info->autocenter,          sizeof(bool), 1, f_target);
    fwrite(&model_info->lock_autocenter,     sizeof(bool), 1, f_target);
    fwrite(&model_info->can_occlude,         sizeof(bool), 1, f_target);
    fwrite(&model_info->can_be_occluded,     sizeof(bool), 1, f_target);
    // fwrite(&model_info->ai_cover,            sizeof(bool), 1, f_target);  // v73
    fwrite(&model_info->skeleton->ht_min,    sizeof(float), 1, f_target);
    fwrite(&model_info->skeleton->ht_max,    sizeof(float), 1, f_target);
    fwrite(&model_info->skeleton->af_max,    sizeof(float), 1, f_target);
    fwrite(&model_info->skeleton->mf_max,    sizeof(float), 1, f_target);
    fwrite(&model_info->skeleton->mf_act,    sizeof(float), 1, f_target);
    fwrite(&model_info->skeleton->t_body,    sizeof(float), 1, f_target);
    fwrite(&model_info->force_not_alpha,     sizeof(bool), 1, f_target);
    fwrite(&model_info->sb_source,           sizeof(int32_t), 1, f_target);
    fwrite(&model_info->prefer_shadow_volume, sizeof(bool), 1, f_target);
    fwrite(&model_info->shadow_offset,       sizeof(float), 1, f_target);
    fwrite(&model_info->animated,            sizeof(bool), 1, f_target);
    write_skeleton(f_target, model_info->skeleton);
    fwrite(&model_info->map_type,            sizeof(char), 1, f_target);
    fwrite(&model_info->n_floats,            sizeof(uint32_t), 1, f_target);
    //fwrite("\0\0\0\0\0", 4, 1, f_target); // compression header for empty array
    fwrite(&model_info->mass,                sizeof(float), 1, f_target);
    fwrite(&model_info->mass_reciprocal,     sizeof(float), 1, f_target);
    fwrite(&model_info->armor,               sizeof(float), 1, f_target);
    fwrite(&model_info->inv_armor,           sizeof(float), 1, f_target);
    fwrite(&model_info->special_lod_indices, sizeof(struct lod_indices), 1, f_target);
    fwrite(&model_info->min_shadow,          sizeof(uint32_t), 1, f_target);
    fwrite(&model_info->can_blend,           sizeof(bool), 1, f_target);
    fwrite(&model_info->class_type,          sizeof(char), 1, f_target);
    fwrite(&model_info->destruct_type,       sizeof(char), 1, f_target);
    fwrite(&model_info->property_frequent,   sizeof(bool), 1, f_target);
    fwrite(&model_info->always_0,            sizeof(uint32_t), 1, f_target); //@todo Array of unused Selection Names

    // v73 adds another 4 bytes here

    //sets preferredShadowVolumeLod, preferredShadowBufferLod, and preferredShadowBufferLodVis for each LOD
    for (i = 0; i < num_lods; i++)
        fwrite("\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff", 12, 1, f_target);
}


void write_odol_section(FILE *f_target, struct odol_section *odol_section) {
    fwrite(&odol_section->face_index_start,     sizeof(uint32_t), 1, f_target);
    fwrite(&odol_section->face_index_end,       sizeof(uint32_t), 1, f_target);
    fwrite(&odol_section->min_bone_index,       sizeof(uint32_t), 1, f_target);
    fwrite(&odol_section->bones_count,          sizeof(uint32_t), 1, f_target);
    fwrite(&odol_section->mat_dummy,            sizeof(uint32_t), 1, f_target);
    fwrite(&odol_section->common_texture_index, sizeof(uint16_t), 1, f_target);
    fwrite(&odol_section->common_face_flags,    sizeof(uint32_t), 1, f_target);
    fwrite(&odol_section->material_index,       sizeof(int32_t), 1, f_target);
    if (odol_section->material_index == -1)
        fputc(0, f_target);
    fwrite(&odol_section->num_stages,           sizeof(uint32_t), 1, f_target);
    fwrite(odol_section->area_over_tex,         sizeof(float), odol_section->num_stages, f_target);
    fwrite(&odol_section->unknown_long,         sizeof(uint32_t), 1, f_target);
}


void write_odol_selection(FILE *f_target, struct odol_selection *odol_selection) {
    fwrite(odol_selection->name, strlen(odol_selection->name) + 1, 1, f_target);

    fwrite(&odol_selection->num_faces, sizeof(uint32_t), 1, f_target);
    if (odol_selection->num_faces > 0) {
        fputc(0, f_target);
        fwrite(odol_selection->faces, sizeof(uint32_t) * odol_selection->num_faces, 1, f_target);
    }

    fwrite(&odol_selection->always_0, sizeof(uint32_t), 1, f_target);

    fwrite(&odol_selection->is_sectional, 1, 1, f_target);
    fwrite(&odol_selection->num_sections, sizeof(uint32_t), 1, f_target);
    if (odol_selection->num_sections > 0) {
        fputc(0, f_target);
        fwrite(odol_selection->sections, sizeof(uint32_t) * odol_selection->num_sections, 1, f_target);
    }

    fwrite(&odol_selection->num_vertices, sizeof(uint32_t), 1, f_target);
    if (odol_selection->num_vertices > 0) {
        fputc(0, f_target);
        fwrite(odol_selection->vertices, sizeof(uint32_t) * odol_selection->num_vertices, 1, f_target);
    }

    fwrite(&odol_selection->num_vertex_weights, sizeof(uint32_t), 1, f_target);
    if (odol_selection->num_vertex_weights > 0) {
        fputc(0, f_target);
        fwrite(odol_selection->vertex_weights, sizeof(uint8_t) * odol_selection->num_vertex_weights, 1, f_target);
    }
}


void write_material(FILE *f_target, struct material *material) {
    int i;

    fwrite( material->path, strlen(material->path) + 1, 1, f_target);
    fwrite(&material->type, sizeof(uint32_t), 1, f_target);
    fwrite(&material->emissive, sizeof(struct color), 1, f_target);
    fwrite(&material->ambient, sizeof(struct color), 1, f_target);
    fwrite(&material->diffuse, sizeof(struct color), 1, f_target);
    fwrite(&material->forced_diffuse, sizeof(struct color), 1, f_target);
    fwrite(&material->specular, sizeof(struct color), 1, f_target);
    fwrite(&material->specular2, sizeof(struct color), 1, f_target);
    fwrite(&material->specular_power, sizeof(float), 1, f_target);
    fwrite(&material->pixelshader_id, sizeof(uint32_t), 1, f_target);
    fwrite(&material->vertexshader_id, sizeof(uint32_t), 1, f_target);
    fwrite(&material->depr_1, sizeof(uint32_t), 1, f_target);
    fwrite(&material->depr_2, sizeof(uint32_t), 1, f_target);
    fwrite( material->surface, strlen(material->surface) + 1, 1, f_target);
    fwrite(&material->depr_3, sizeof(uint32_t), 1, f_target);
    fwrite(&material->render_flags, sizeof(uint32_t), 1, f_target);
    fwrite(&material->num_textures, sizeof(uint32_t), 1, f_target);
    fwrite(&material->num_transforms, sizeof(uint32_t), 1, f_target);

    for (i = 0; i < material->num_textures; i++) {
        fwrite(&material->textures[i].texture_filter, sizeof(uint32_t), 1, f_target);
        fwrite( material->textures[i].path, strlen(material->textures[i].path) + 1, 1, f_target);
        fwrite(&material->textures[i].transform_index, sizeof(uint32_t), 1, f_target);
        fwrite(&material->dummy_texture.type11_bool, sizeof(bool), 1, f_target);
    }

    fwrite( material->transforms, sizeof(struct stage_transform), material->num_transforms, f_target);

    fwrite(&material->dummy_texture.texture_filter, sizeof(uint32_t), 1, f_target);
    fwrite( material->dummy_texture.path, strlen(material->dummy_texture.path) + 1, 1, f_target);
    fwrite(&material->dummy_texture.transform_index, sizeof(uint32_t), 1, f_target);
    fwrite(&material->dummy_texture.type11_bool, sizeof(bool), 1, f_target);
}


void write_odol_lod(FILE *f_target, struct odol_lod *odol_lod) {
    short u, v;
    int x, y, z;
    long i;
    long fp_vertextable_size;
    uint32_t temp;
    char *ptr;
    float u_relative;
    float v_relative;

    fwrite(&odol_lod->num_proxies, sizeof(uint32_t), 1, f_target);
    for (i = 0; i < odol_lod->num_proxies; i++) {
        fwrite( odol_lod->proxies[i].name, strlen(odol_lod->proxies[i].name) + 1, 1, f_target);
        fwrite(&odol_lod->proxies[i].transform_x, sizeof(struct triplet), 1, f_target);
        fwrite(&odol_lod->proxies[i].transform_y, sizeof(struct triplet), 1, f_target);
        fwrite(&odol_lod->proxies[i].transform_z, sizeof(struct triplet), 1, f_target);
        fwrite(&odol_lod->proxies[i].transform_n, sizeof(struct triplet), 1, f_target);
        fwrite(&odol_lod->proxies[i].proxy_id, sizeof(uint32_t), 1, f_target);
        fwrite(&odol_lod->proxies[i].selection_index, sizeof(uint32_t), 1, f_target);
        fwrite(&odol_lod->proxies[i].bone_index, sizeof(int32_t), 1, f_target);
        fwrite(&odol_lod->proxies[i].section_index, sizeof(uint32_t), 1, f_target);
    }

    fwrite(&odol_lod->num_bones_subskeleton, sizeof(uint32_t), 1, f_target);
    fwrite( odol_lod->subskeleton_to_skeleton, sizeof(uint32_t) * odol_lod->num_bones_subskeleton, 1, f_target);

    fwrite(&odol_lod->num_bones_skeleton, sizeof(uint32_t), 1, f_target);
    for (i = 0; i < odol_lod->num_bones_skeleton; i++) {
        fwrite(&odol_lod->skeleton_to_subskeleton[i].num_links, sizeof(uint32_t), 1, f_target);
        fwrite( odol_lod->skeleton_to_subskeleton[i].links, sizeof(uint32_t) * odol_lod->skeleton_to_subskeleton[i].num_links, 1, f_target);
    }

    fwrite(&odol_lod->num_points, sizeof(uint32_t), 1, f_target);
    fwrite(&odol_lod->face_area, sizeof(float), 1, f_target);
    fwrite( odol_lod->clip_flags, sizeof(uint32_t), 2, f_target);
    fwrite(&odol_lod->min_pos, sizeof(struct triplet), 1, f_target);
    fwrite(&odol_lod->max_pos, sizeof(struct triplet), 1, f_target);
    fwrite(&odol_lod->autocenter_pos, sizeof(struct triplet), 1, f_target);
    fwrite(&odol_lod->sphere, sizeof(float), 1, f_target);

    fwrite(&odol_lod->num_textures, sizeof(uint32_t), 1, f_target);
    ptr = odol_lod->textures;
    for (i = 0; i < odol_lod->num_textures; i++)
        ptr += strlen(ptr) + 1;
    fwrite( odol_lod->textures, ptr - odol_lod->textures, 1, f_target);

    fwrite(&odol_lod->num_materials, sizeof(uint32_t), 1, f_target);
    for (i = 0; i < odol_lod->num_materials; i++)
        write_material(f_target, &odol_lod->materials[i]);

    // the point-to-vertex and vertex-to-point arrays are just left out
    fwrite("\0\0\0\0\0\0\0\0", sizeof(uint32_t) * 2, 1, f_target);

    fwrite(&odol_lod->num_faces, sizeof(uint32_t), 1, f_target);
    fwrite(&odol_lod->face_allocation_size, sizeof(uint32_t), 1, f_target);
    fwrite(&odol_lod->always_0, sizeof(uint16_t), 1, f_target);

    for (i = 0; i < odol_lod->num_faces; i++) {
        fwrite(&odol_lod->faces[i].face_type, sizeof(uint8_t), 1, f_target);
        fwrite( odol_lod->faces[i].table, sizeof(uint32_t) * odol_lod->faces[i].face_type, 1, f_target);
    }

    fwrite(&odol_lod->num_sections, sizeof(uint32_t), 1, f_target);
    for (i = 0; i < odol_lod->num_sections; i++) {
        write_odol_section(f_target, &odol_lod->sections[i]);
    }

    fwrite(&odol_lod->num_selections, sizeof(uint32_t), 1, f_target);
    for (i = 0; i < odol_lod->num_selections; i++) {
        write_odol_selection(f_target, &odol_lod->selections[i]);
    }

    fwrite(&odol_lod->num_properties, sizeof(uint32_t), 1, f_target);
    for (i = 0; i < odol_lod->num_properties; i++) {
        fwrite(odol_lod->properties[i].name, strlen(odol_lod->properties[i].name) + 1, 1, f_target);
        fwrite(odol_lod->properties[i].value, strlen(odol_lod->properties[i].value) + 1, 1, f_target);
    }

    fwrite(&odol_lod->num_frames, sizeof(uint32_t), 1, f_target);
    // @todo frames

    fwrite(&odol_lod->icon_color, sizeof(uint32_t), 1, f_target);
    fwrite(&odol_lod->selected_color, sizeof(uint32_t), 1, f_target);
    fwrite(&odol_lod->flags, sizeof(uint32_t), 1, f_target);
    fwrite(&odol_lod->vertexboneref_is_simple, sizeof(bool), 1, f_target);

    fp_vertextable_size = ftell(f_target);
    fwrite("\0\0\0\0", 4, 1, f_target);

    // pointflags
    fwrite(&odol_lod->num_points, 4, 1, f_target);
    fputc(1, f_target);
    if (odol_lod->num_points > 0)
        fwrite("\0\0\0\0", 4, 1, f_target);

    // uvs
    fwrite( odol_lod->uv_scale, sizeof(struct uv_pair) * 2, 1, f_target);
    fwrite(&odol_lod->num_points, sizeof(uint32_t), 1, f_target);
    fputc(0, f_target);
    if (odol_lod->num_points > 0) {
        fputc(0, f_target);
        for (i = 0; i < odol_lod->num_points; i++) {
            // write compressed pair
            u_relative = (odol_lod->uv_coords[i].u - odol_lod->uv_scale[0].u) / (odol_lod->uv_scale[1].u - odol_lod->uv_scale[0].u);
            v_relative = (odol_lod->uv_coords[i].v - odol_lod->uv_scale[0].v) / (odol_lod->uv_scale[1].v - odol_lod->uv_scale[0].v);
            u = (short)(u_relative * 2 * INT16_MAX - INT16_MAX);
            v = (short)(v_relative * 2 * INT16_MAX - INT16_MAX);

            fwrite(&u, sizeof(int16_t), 1, f_target);
            fwrite(&v, sizeof(int16_t), 1, f_target);
        }
    }
    fwrite("\x01\0\0\0", 4, 1, f_target);

    // points
    fwrite(&odol_lod->num_points, sizeof(uint32_t), 1, f_target);
    if (odol_lod->num_points > 0) {
        fputc(0, f_target);
        fwrite( odol_lod->points, sizeof(struct triplet) * odol_lod->num_points, 1, f_target);
    }

    // normals
    fwrite(&odol_lod->num_points, sizeof(uint32_t), 1, f_target);
    fputc(0, f_target);
    if (odol_lod->num_points > 0) {
        fputc(0, f_target);
        for (i = 0; i < odol_lod->num_points; i++) {
            // write compressed triplet
            x = (int)(-511.0f * odol_lod->normals[i].x + 0.5);
            y = (int)(-511.0f * odol_lod->normals[i].y + 0.5);
            z = (int)(-511.0f * odol_lod->normals[i].z + 0.5);

            x = MAX(MIN(x, 511), -511);
            y = MAX(MIN(y, 511), -511);
            z = MAX(MIN(z, 511), -511);

            temp = (((uint32_t)z & 0x3FF) << 20) | (((uint32_t)y & 0x3FF) << 10) | ((uint32_t)x & 0x3FF);
            fwrite(&temp, sizeof(uint32_t), 1, f_target);
        }
    }

    // ST coordinates
    fwrite("\0\0\0\0", 4, 1, f_target);

    // vertex bone ref
    if (odol_lod->vertexboneref == 0 || odol_lod->num_points == 0) {
        fwrite("\0\0\0\0", 4, 1, f_target);
    } else {
        fwrite(&odol_lod->num_points, sizeof(uint32_t), 1, f_target);
        fputc(0, f_target);
        fwrite(odol_lod->vertexboneref, sizeof(struct odol_vertexboneref), odol_lod->num_points, f_target);
    }

    // neighbor bone ref
    fwrite("\0\0\0\0", 4, 1, f_target);

    temp = ftell(f_target) - fp_vertextable_size;
    fseek(f_target, fp_vertextable_size, SEEK_SET);
    fwrite(&temp, 4, 1, f_target);
    fseek(f_target, 0, SEEK_END);
    
    // has Collimator info?
    fwrite("\0\0\0\0", sizeof(uint32_t), 1, f_target); //If 1 then need to write CollimatorInfo structure

    // unknown byte
    fwrite("\0", 1, 1, f_target);
}


void calculate_axis(struct animation *anim, uint32_t num_lods, struct mlod_lod *mlod_lods) {
    /*
     * Gets the absolute axis position and direction for the given rotation
     * or translation animations.
     *
     * At the moment, all axis selections are expected to be in the memory LOD.
     */

    int i;
    int j;
    int k;

    anim->axis_pos = empty_vector;
    anim->axis_dir = empty_vector;

    if (anim->axis[0] == 0 && anim->begin[0] == 0 && anim->end[0] == 0)
        return;

    for (i = 0; i < num_lods; i++) {
        if (float_equal(mlod_lods[i].resolution, LOD_MEMORY, 0.01))
            break;
    }
    if (i == num_lods)
        return;

    for (j = 0; j < mlod_lods[i].num_selections; j++) {
        if (anim->axis[0] == 0) {
            if (stricmp(mlod_lods[i].selections[j].name, anim->begin) == 0) {
                for (k = 0; k < mlod_lods[i].num_points; k++) {
                    if (mlod_lods[i].selections[j].points[k] > 0) {
                        memcpy(&anim->axis_pos, &mlod_lods[i].points[k], sizeof(struct triplet));
                        break;
                    }
                }
            }
            if (stricmp(mlod_lods[i].selections[j].name, anim->end) == 0) {
                for (k = 0; k < mlod_lods[i].num_points; k++) {
                    if (mlod_lods[i].selections[j].points[k] > 0) {
                        memcpy(&anim->axis_dir, &mlod_lods[i].points[k], sizeof(struct triplet));
                        break;
                    }
                }
            }
        } else if (stricmp(mlod_lods[i].selections[j].name, anim->axis) == 0) {
            for (k = 0; k < mlod_lods[i].num_points; k++) {
                if (mlod_lods[i].selections[j].points[k] > 0) {
                    memcpy(&anim->axis_pos, &mlod_lods[i].points[k], sizeof(struct triplet));
                    break;
                }
            }
            for (k = k + 1; k < mlod_lods[i].num_points; k++) {
                if (mlod_lods[i].selections[j].points[k] > 0) {
                    memcpy(&anim->axis_dir, &mlod_lods[i].points[k], sizeof(struct triplet));
                    break;
                }
            }
        }
    }

    anim->axis_dir = vector_sub(anim->axis_dir, anim->axis_pos);

    if (anim->type >= TYPE_ROTATION_X && anim->type <= TYPE_ROTATION_Z) {
        anim->axis_pos = vector_add(anim->axis_pos, vector_mult_scalar(0.5, anim->axis_dir));
        anim->axis_dir = empty_vector;
        anim->axis_dir.x = 1.0f;
    } else if (anim->type == TYPE_ROTATION) {
        anim->axis_dir = vector_normalize(anim->axis_dir);
    } else {
        anim->axis_pos = empty_vector;
    }
}


void write_animations(FILE *f_target, uint32_t num_lods, struct mlod_lod *mlod_lods,
        struct model_info *model_info) {
    int i;
    int j;
    int k;
    uint32_t num;
    int32_t index;
    struct animation *anim;

    // Write animation classes
    fwrite(&model_info->skeleton->num_animations, sizeof(uint32_t), 1, f_target);
    for (i = 0; i < model_info->skeleton->num_animations; i++) {
        anim = &model_info->skeleton->animations[i];
        fwrite(&anim->type, sizeof(uint32_t), 1, f_target);
        fwrite( anim->name, strlen(anim->name) + 1, 1, f_target);
        fwrite( anim->source, strlen(anim->source) + 1, 1, f_target);
        fwrite(&anim->min_value, sizeof(float), 1, f_target);
        fwrite(&anim->max_value, sizeof(float), 1, f_target);
        fwrite(&anim->min_value, sizeof(float), 1, f_target);
        fwrite(&anim->max_value, sizeof(float), 1, f_target);
        //fwrite(&anim->min_phase, sizeof(float), 1, f_target);
        //fwrite(&anim->max_phase, sizeof(float), 1, f_target);
        fwrite(&anim->junk, sizeof(uint32_t), 1, f_target);
        fwrite(&anim->always_0, sizeof(uint32_t), 1, f_target);
        fwrite(&anim->source_address, sizeof(uint32_t), 1, f_target);

        switch (anim->type) {
            case TYPE_ROTATION:
            case TYPE_ROTATION_X:
            case TYPE_ROTATION_Y:
            case TYPE_ROTATION_Z:
                fwrite(&anim->angle0, sizeof(float), 1, f_target);
                fwrite(&anim->angle1, sizeof(float), 1, f_target);
                break;
            case TYPE_TRANSLATION:
            case TYPE_TRANSLATION_X:
            case TYPE_TRANSLATION_Y:
            case TYPE_TRANSLATION_Z:
                fwrite(&anim->offset0, sizeof(float), 1, f_target);
                fwrite(&anim->offset1, sizeof(float), 1, f_target);
                break;
            case TYPE_DIRECT:
                fwrite(&anim->axis_pos, sizeof(struct triplet), 1, f_target);
                fwrite(&anim->axis_dir, sizeof(struct triplet), 1, f_target);
                fwrite(&anim->angle, sizeof(float), 1, f_target);
                fwrite(&anim->axis_offset, sizeof(float), 1, f_target);
                break;
            case TYPE_HIDE:
                fwrite(&anim->hide_value, sizeof(float), 1, f_target);
                fwrite(&anim->unhide_value, sizeof(float), 1, f_target);
                break;
        }
    }

    // Write bone2anim and anim2bone lookup tables
    fwrite(&num_lods, sizeof(uint32_t), 1, f_target);

    // bone2anim
    for (i = 0; i < num_lods; i++) {
        fwrite(&model_info->skeleton->num_bones, sizeof(uint32_t), 1, f_target);
        for (j = 0; j < model_info->skeleton->num_bones; j++) {
            num = 0;
            for (k = 0; k < model_info->skeleton->num_animations; k++) {
                anim = &model_info->skeleton->animations[k];
                if (stricmp(anim->selection, model_info->skeleton->bones[j].name) == 0)
                    num++;
            }

            fwrite(&num, sizeof(uint32_t), 1, f_target);

            for (k = 0; k < model_info->skeleton->num_animations; k++) {
                anim = &model_info->skeleton->animations[k];
                if (stricmp(anim->selection, model_info->skeleton->bones[j].name) == 0) {
                    num = (uint32_t)k;
                    fwrite(&num, sizeof(uint32_t), 1, f_target);
                }
            }
        }
    }

    // anim2bone
    for (i = 0; i < num_lods; i++) {
        for (j = 0; j < model_info->skeleton->num_animations; j++) {
            anim = &model_info->skeleton->animations[j];

            index = -1;
            for (k = 0; k < model_info->skeleton->num_bones; k++) {
                if (stricmp(anim->selection, model_info->skeleton->bones[k].name) == 0) {
                    index = (int32_t)k;
                    break;
                }
            }

            fwrite(&index, sizeof(int32_t), 1, f_target);

            if (index == -1) {
                if (i == 0) { // we only report errors for the first LOD
                    lnwarningf(current_target, -1, "unknown-bone", "Failed to find bone \"%s\" for animation \"%s\".\n",
                            model_info->skeleton->bones[k].name, anim->name);
                }
                continue;
            }

            if (anim->type >= 8)
                continue;

            calculate_axis(anim, num_lods, mlod_lods);

            if (model_info->autocenter)
                anim->axis_pos = vector_sub(anim->axis_pos, model_info->centre_of_mass);

            fwrite(&anim->axis_pos, sizeof(struct triplet), 1, f_target);
            fwrite(&anim->axis_dir, sizeof(struct triplet), 1, f_target);
        }
    }
}


int mlod2odol(char *source, char *target) {
    /*
     * Converts the MLOD P3D to ODOL. Overwrites the target if it already
     * exists.
     *
     * Returns 0 on success and a positive integer on failure.
     */

    extern struct arguments args;
    extern char *current_target;
    FILE *f_source;
    FILE *f_temp;
    FILE *f_target;
    char buffer[4096];
    int datasize;
    int i;
    int j;
    int success;
    long fp_lods;
    long fp_temp;
    uint32_t version;
    uint32_t num_lods;
    struct mlod_lod *mlod_lods;
    struct model_info model_info;
    struct odol_lod odol_lod;

    current_target = source;

#ifdef _WIN32
    char temp_name[2048];
    if (!GetTempFileName(".", "amk", 0, temp_name)) {
        errorf("Failed to get temp file name (system error %i).\n", GetLastError());
        return 1;
    }
    f_temp = fopen(temp_name, "wb+");
#else
    f_temp = tmpfile();
#endif

    if (!f_temp) {
        errorf("Failed to open temp file.\n");
#ifdef _WIN32
        DeleteFile(temp_name);
#endif
        return 1;
    }

    // Open source and read LODs
    f_source = fopen(source, "rb");
    if (!f_source) {
        errorf("Failed to open source file.\n");
        fclose(f_temp);
#ifdef _WIN32
        DeleteFile(temp_name);
#endif
        return 2;
    }

    fgets(buffer, 5, f_source);
    if (strncmp(buffer, "MLOD", 4) != 0) {
        if (strcmp(args.positionals[0], "binarize") == 0)
            errorf("Source file is not MLOD.\n");
        fclose(f_temp);
        fclose(f_source);
#ifdef _WIN32
        DeleteFile(temp_name);
#endif
        return -3;
    }

    fseek(f_source, 8, SEEK_SET);
    fread(&num_lods, 4, 1, f_source);
    mlod_lods = (struct mlod_lod *)safe_malloc(sizeof(struct mlod_lod) * num_lods);
    num_lods = read_lods(f_source, mlod_lods, num_lods);
    if (num_lods < 0) {
        errorf("Failed to read LODs.\n");
        free(mlod_lods);
        fclose(f_temp);
        fclose(f_source);
#ifdef _WIN32
        DeleteFile(temp_name);
#endif
        return 4;
    }

    fclose(f_source);

    // Write header
    fwrite("ODOL", 4, 1, f_temp);
    version = P3DVERSION;
    fwrite(&version, sizeof(uint32_t), 1, f_temp); // version 70
    fwrite("\0\0\0\0", sizeof(uint32_t), 1, f_temp); // AppID
    fwrite("\0", 1, 1, f_temp); // muzzleFlash string
    fwrite(&num_lods, 4, 1, f_temp);

    // Write model info
    build_model_info(mlod_lods, num_lods, &model_info);
    success = read_model_config(source, model_info.skeleton);
    if (success > 0) {
        errorf("Failed to read model config.\n");
        fclose(f_temp);
#ifdef _WIN32
        DeleteFile(temp_name);
#endif
        return success;
    }

    current_target = source;

    write_model_info(f_temp, num_lods, &model_info);

    // Write animations
    if (model_info.skeleton->num_animations > 0) {
        fputc(1, f_temp);
        write_animations(f_temp, num_lods, mlod_lods, &model_info);
    } else {
        fputc(0, f_temp);
    }

    // Write place holder LOD addresses
    fp_lods = ftell(f_temp);
    for (i = 0; i < num_lods; i++)
        fwrite("\0\0\0\0\0\0\0\0", 8, 1, f_temp);

    // Write LOD face defaults (or rather, don't)
    for (i = 0; i < num_lods; i++)
        fputc(1, f_temp);

    // Write LODs
    for (i = 0; i < num_lods; i++) {
        // Write start address
        fp_temp = ftell(f_temp);
        fseek(f_temp, fp_lods + i * 4, SEEK_SET);
        fwrite(&fp_temp, 4, 1, f_temp);
        fseek(f_temp, 0, SEEK_END);

        // Convert to ODOL
        convert_lod(&mlod_lods[i], &odol_lod, &model_info);

        // Write to file
        write_odol_lod(f_temp, &odol_lod);

        // Clean up
        free(odol_lod.proxies);
        free(odol_lod.subskeleton_to_skeleton);
        free(odol_lod.skeleton_to_subskeleton);
        free(odol_lod.textures);
        free(odol_lod.point_to_vertex);
        free(odol_lod.vertex_to_point);
        free(odol_lod.face_lookup);
        free(odol_lod.faces);
        free(odol_lod.uv_coords);
        free(odol_lod.points);
        free(odol_lod.normals);
        free(odol_lod.sections);
        free(odol_lod.vertexboneref);

        for (j = 0; j < odol_lod.num_materials; j++) {
            free(odol_lod.materials[j].textures);
            free(odol_lod.materials[j].transforms);
        }

        free(odol_lod.materials);

        for (j = 0; j < odol_lod.num_selections; j++) {
            free(odol_lod.selections[j].faces);
            free(odol_lod.selections[j].sections);
            free(odol_lod.selections[j].vertices);
            free(odol_lod.selections[j].vertex_weights);
        }

        free(odol_lod.selections);

        // Write end address
        fp_temp = ftell(f_temp);
        fseek(f_temp, fp_lods + (num_lods + i) * 4, SEEK_SET);
        fwrite(&fp_temp, 4, 1, f_temp);
        fseek(f_temp, 0, SEEK_END);
    }

    // Write PhysX (@todo)
    fwrite("\x00\x03\x03\x03\x00\x00\x00\x00", 8, 1, f_temp);
    fwrite("\x00\x03\x03\x03\x00\x00\x00\x00", 8, 1, f_temp);
    fwrite("\x00\x00\x00\x00\x00\x03\x03\x03", 8, 1, f_temp);
    fwrite("\x00\x00\x00\x00\x00\x03\x03\x03", 8, 1, f_temp);
    fwrite("\x00\x00\x00\x00", 4, 1, f_temp);

    // Write temp to target
    fseek(f_temp, 0, SEEK_END);
    datasize = ftell(f_temp);

    f_target = fopen(target, "wb");
    if (!f_target) {
        errorf("Failed to open target file.\n");
        fclose(f_temp);
#ifdef _WIN32
        DeleteFile(temp_name);
#endif
        return 5;
    }

    fseek(f_temp, 0, SEEK_SET);
    for (i = 0; datasize - i >= sizeof(buffer); i += sizeof(buffer)) {
        fread(buffer, sizeof(buffer), 1, f_temp);
        fwrite(buffer, sizeof(buffer), 1, f_target);
    }
    fread(buffer, datasize - i, 1, f_temp);
    fwrite(buffer, datasize - i, 1, f_target);

    // Clean up
    fclose(f_temp);
    fclose(f_target);

#ifdef _WIN32
    DeleteFile(temp_name);
#endif

    for (i = 0; i < num_lods; i++) {
        free(mlod_lods[i].points);
        free(mlod_lods[i].facenormals);
        free(mlod_lods[i].faces);
        free(mlod_lods[i].mass);
        free(mlod_lods[i].sharp_edges);

        for (j = 0; j < mlod_lods[i].num_selections; j++) {
            free(mlod_lods[i].selections[j].points);
            free(mlod_lods[i].selections[j].faces);
        }

        free(mlod_lods[i].selections);
    }
    free(mlod_lods);

    free(model_info.lod_resolutions);
    free(model_info.skeleton);

    return 0;
}
