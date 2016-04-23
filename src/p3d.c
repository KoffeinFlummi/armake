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


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "docopt.h"
#include "filesystem.h"
#include "utils.h"
#include "model_config.h"
#include "p3d.h"


int read_lods(FILE *f_source, struct mlod_lod *mlod_lods, uint32_t num_lods) {
    /*
     * Reads all LODs (starting at the current position of f_source) into
     * the given LODs array.
     *
     * Returns a 0 on success and a positive integer on failure.
     */

    char buffer[256];
    int i;
    int j;
    int fp_tmp;
    int fp_taggs;
    uint32_t tagg_len;

    fseek(f_source, 12, SEEK_SET);

    for (i = 0; i < num_lods; i++) {
        fread(buffer, 4, 1, f_source);
        if (strncmp(buffer, "P3DM", 4) != 0)
            return 1;

        fseek(f_source, 8, SEEK_CUR);
        fread(&mlod_lods[i].num_points, 4, 1, f_source);
        fread(&mlod_lods[i].num_facenormals, 4, 1, f_source);
        fread(&mlod_lods[i].num_faces, 4, 1, f_source);
        fseek(f_source, 4, SEEK_CUR);

        mlod_lods[i].points = (struct point *)malloc(sizeof(struct point) * mlod_lods[i].num_points);
        for (j = 0; j < mlod_lods[i].num_points; j++)
            fread(&mlod_lods[i].points[j], sizeof(struct point), 1, f_source);

        mlod_lods[i].facenormals = (struct triplet *)malloc(sizeof(struct triplet) * mlod_lods[i].num_facenormals);
        for (j = 0; j < mlod_lods[i].num_facenormals; j++)
            fread(&mlod_lods[i].facenormals[j], sizeof(struct triplet), 1, f_source);

        mlod_lods[i].faces = (struct mlod_face *)malloc(sizeof(struct mlod_face) * mlod_lods[i].num_faces);
        int p = 0;
        for (j = 0; j < mlod_lods[i].num_faces; j++) {
            fread(&mlod_lods[i].faces[j], 72, 1, f_source);
            p += mlod_lods[i].faces[j].face_type;

            fp_tmp = ftell(f_source);
            fread(mlod_lods[i].faces[j].texture_name,
                sizeof(mlod_lods[i].faces[j].texture_name), 1, f_source);
            fseek(f_source, fp_tmp + strlen(mlod_lods[i].faces[j].texture_name) + 1, SEEK_SET);

            fp_tmp = ftell(f_source);
            fread(mlod_lods[i].faces[j].material_name,
                sizeof(mlod_lods[i].faces[j].material_name), 1, f_source);
            fseek(f_source, fp_tmp + strlen(mlod_lods[i].faces[j].material_name) + 1, SEEK_SET);
        }

        fread(buffer, 4, 1, f_source);
        if (strncmp(buffer, "TAGG", 4) != 0)
            return 2;

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

        mlod_lods[i].selections = (struct mlod_selection *)malloc(sizeof(struct mlod_selection) * mlod_lods[i].num_selections);
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

                mlod_lods[i].selections[j].points = (uint8_t *)malloc(mlod_lods[i].num_points);
                mlod_lods[i].selections[j].faces = (uint8_t *)malloc(mlod_lods[i].num_faces);

                fread(mlod_lods[i].selections[j].points, mlod_lods[i].num_points, 1, f_source);
                fread(mlod_lods[i].selections[j].faces, mlod_lods[i].num_faces, 1, f_source);
            }

            if (strcmp(buffer, "#Mass#") == 0) {
                mlod_lods[i].mass = (float *)malloc(sizeof(float) * mlod_lods[i].num_points);
                fread(mlod_lods[i].mass, sizeof(float) * mlod_lods[i].num_points, 1, f_source);
            }

            if (strcmp(buffer, "#SharpEdges#") == 0) {
                mlod_lods[i].num_sharp_edges = tagg_len / (2 * sizeof(uint32_t));
                mlod_lods[i].sharp_edges = (uint32_t *)malloc(tagg_len);
                fread(mlod_lods[i].sharp_edges, tagg_len, 1, f_source);
            }

            if (strcmp(buffer, "#Property#") == 0) {
                for (j = 0; j < MAXPROPERTIES; j++) {
                    if (mlod_lods[i].properties[j].name[0] == 0)
                        break;
                }
                if (j == MAXPROPERTIES)
                    return 3;

                fread(mlod_lods[i].properties[j].name, 64, 1, f_source);
                fread(mlod_lods[i].properties[j].value, 64, 1, f_source);
            }

            fseek(f_source, fp_tmp, SEEK_SET);

            if (strcmp(buffer, "#EndOfFile#") == 0)
                break;
        }

        fread(&mlod_lods[i].resolution, 4, 1, f_source);
    }

    return 0;
}


void get_centre_of_mass(struct mlod_lod *mlod_lods, uint32_t num_lods, struct triplet *centre_of_mass) {
    /*
     * Calculate the centre of mass for the given LODs and stores
     * it in the given triplet struct.
     */

    int i;
    int j;
    float mass;
    float x;
    float y;
    float z;

    centre_of_mass->x = 0;
    centre_of_mass->y = 0;
    centre_of_mass->z = 0;

    for (i = 0; i < num_lods; i++) {
        if (float_equal(mlod_lods[i].resolution, LOD_GEOMETRY, 0.01))
            break;
    }

    if (i == num_lods)
        return;

    if (mlod_lods[i].mass == 0)
        return;

    mass = 0;
    for (j = 0; j < mlod_lods[i].num_points; j++) {
        mass += mlod_lods[i].mass[j];
    }

    for (j = 0; j < mlod_lods[i].num_points; j++) {
        x = mlod_lods[i].points[j].x;
        y = mlod_lods[i].points[j].y;
        z = mlod_lods[i].points[j].z;

        centre_of_mass->x += x * (mlod_lods[i].mass[j] / mass);
        centre_of_mass->y += y * (mlod_lods[i].mass[j] / mass);
        centre_of_mass->z += z * (mlod_lods[i].mass[j] / mass);
    }
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

    bbox_min->x = 0;
    bbox_min->y = 0;
    bbox_min->z = 0;
    bbox_max->x = 0;
    bbox_max->y = 0;
    bbox_max->z = 0;

    first = true;

    for (i = 0; i < num_lods; i++) {
        if (mlod_lods[i].resolution > LOD_GRAPHICAL && visual_only)
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


float get_sphere(struct mlod_lod *mlod_lod, struct triplet *centre_of_mass) {
    /*
     * Calculate and return the bounding sphere for the given LOD.
     */

    long i;
    float sphere;
    float dist;

    sphere = 0;
    for (i = 0; i < mlod_lod->num_points; i++) {
        dist = (float)sqrt((double)(
            (mlod_lod->points[i].x - centre_of_mass->x) * (mlod_lod->points[i].x - centre_of_mass->x) +
            (mlod_lod->points[i].y - centre_of_mass->y) * (mlod_lod->points[i].y - centre_of_mass->y) +
            (mlod_lod->points[i].z - centre_of_mass->z) * (mlod_lod->points[i].z - centre_of_mass->z)));
        if (dist > sphere)
            sphere = dist;
    }

    return sphere;
}


void build_model_info(struct mlod_lod *mlod_lods, uint32_t num_lods, struct model_info *model_info) {
    int i;
    int j;
    float sphere;
    struct triplet bbox_total_min;
    struct triplet bbox_total_max;

    model_info->lod_resolutions = (float *)malloc(sizeof(float) * num_lods);

    for (i = 0; i < num_lods; i++)
        model_info->lod_resolutions[i] = mlod_lods[i].resolution;

    model_info->index = 0;

    // Bounding box & center
    get_bounding_box(mlod_lods, num_lods, &bbox_total_min, &bbox_total_max, false, false);

    model_info->bounding_center.x = (bbox_total_min.x + bbox_total_max.x) / 2;
    model_info->bounding_center.y = (bbox_total_min.y + bbox_total_max.y) / 2;
    model_info->bounding_center.z = (bbox_total_min.z + bbox_total_max.z) / 2;

    model_info->bbox_min.x = bbox_total_min.x - model_info->bounding_center.x;
    model_info->bbox_min.y = bbox_total_min.y - model_info->bounding_center.y;
    model_info->bbox_min.z = bbox_total_min.z - model_info->bounding_center.z;
    model_info->bbox_max.x = bbox_total_max.x - model_info->bounding_center.x;
    model_info->bbox_max.y = bbox_total_max.y - model_info->bounding_center.y;
    model_info->bbox_max.z = bbox_total_max.z - model_info->bounding_center.z;

    // Visual bounding box
    get_bounding_box(mlod_lods, num_lods, &model_info->bbox_visual_min, &model_info->bbox_visual_max, true, false);

    model_info->bbox_visual_min.x -= model_info->bounding_center.x;
    model_info->bbox_visual_min.y -= model_info->bounding_center.y;
    model_info->bbox_visual_min.z -= model_info->bounding_center.z;
    model_info->bbox_visual_max.x -= model_info->bounding_center.x;
    model_info->bbox_visual_max.y -= model_info->bounding_center.y;
    model_info->bbox_visual_max.z -= model_info->bounding_center.z;

    // Geometry center
    get_bounding_box(mlod_lods, num_lods, &bbox_total_min, &bbox_total_max, false, true);

    model_info->geometry_center.x = (bbox_total_min.x + bbox_total_max.x) / 2 - model_info->bounding_center.x;
    model_info->geometry_center.y = (bbox_total_min.y + bbox_total_max.y) / 2 - model_info->bounding_center.y;
    model_info->geometry_center.z = (bbox_total_min.z + bbox_total_max.z) / 2 - model_info->bounding_center.z;

    // Centre of mass
    get_centre_of_mass(mlod_lods, num_lods, &model_info->centre_of_mass);

    model_info->centre_of_mass.x -= model_info->bounding_center.x;
    model_info->centre_of_mass.y -= model_info->bounding_center.y;
    model_info->centre_of_mass.z -= model_info->bounding_center.z;

    // Inv inertia (whatever that is) @todo
    model_info->inv_inertia.x = 0;
    model_info->inv_inertia.y = 0;
    model_info->inv_inertia.z = 0;

    // Aiming Center
    // @todo: i think this uses the fire geo lod if available
    model_info->aiming_center = model_info->geometry_center;

    // Spheres
    model_info->mem_lod_sphere = 0.0f;
    model_info->geo_lod_sphere = 0.0f;
    for (i = 0; i < num_lods; i++) {
        sphere = get_sphere(&mlod_lods[i], &model_info->centre_of_mass);
        if (sphere > model_info->mem_lod_sphere)
            model_info->mem_lod_sphere = sphere;
        if (float_equal(mlod_lods[i].resolution, LOD_GEOMETRY, 0.01))
            model_info->geo_lod_sphere = sphere;
    }

    model_info->point_flags[0] = 0;
    model_info->point_flags[1] = 0;
    model_info->point_flags[2] = 0;

    model_info->map_icon_color = 0xff9d8254;
    model_info->map_selected_color = 0xff9d8254;

    model_info->view_density = -100.0f; // @todo

    strncpy(model_info->thermal_profile2,
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
        sizeof(model_info->thermal_profile2)); // @todo

    model_info->autocenter = true; // @todo
    model_info->lock_autocenter = false; // @todo
    model_info->can_occlude = false; // @todo
    model_info->can_be_occluded = true; // @todo
    model_info->allow_animation = false; // @todo

    strncpy(model_info->unknown_flags, "\0\0\0\0\0\0", sizeof(model_info->unknown_flags)); // @todo
    strncpy(model_info->thermal_profile,
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
        sizeof(model_info->thermal_profile)); // @todo

    model_info->unknown_long = 0xff000000;

    model_info->skeleton = (struct skeleton *)malloc(sizeof(struct skeleton));
    model_info->skeleton->num_bones = 0;
    model_info->skeleton->num_sections = 0;
    model_info->skeleton->num_animations = 0;
    model_info->skeleton->ht_min = 0;
    model_info->skeleton->ht_max = 0;
    model_info->skeleton->af_max = 0;
    model_info->skeleton->mf_max = 0;
    model_info->skeleton->mf_act = 0;
    model_info->skeleton->t_body = 0;
    for (i = 0; i < MAXBONES; i++)
        model_info->skeleton->bones[i].name[0] = 0;
    for (i = 0; i < MAXSECTIONS; i++)
        model_info->skeleton->sections[i][0] = 0;
    for (i = 0; i < MAXANIMS; i++)
        model_info->skeleton->animations[i].name[0] = 0;

    model_info->unknown_byte = 0;
    model_info->n_floats = 0;

    model_info->mass = 0;
    for (i = 0; i < num_lods; i++) {
        if (!float_equal(mlod_lods[i].resolution, LOD_GEOMETRY, 0.01))
            continue;
        for (j = 0; j < mlod_lods[i].num_points; j++)
            model_info->mass += mlod_lods[i].mass[j];
    }
    model_info->mass_reciprocal = 10000000000.0f; // @todo
    model_info->alt_mass = 200.0f; // @todo
    model_info->alt_mass_reciprocal = 0.005f; // @todo

    strncpy(model_info->unknown_indices,
        "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff",
        sizeof(model_info->unknown_indices));

    model_info->unknown_long_2 = num_lods;
    model_info->unknown_bool = false;
    model_info->class_type = 0;
    model_info->destruct_type = 0;
    model_info->unknown_bool_2 = false;
    model_info->always_0 = 0;
}


uint32_t add_point(struct odol_lod *odol_lod, struct mlod_lod *mlod_lod,
    uint32_t point_index_mlod, struct triplet *normal, struct uv_pair *uv_coords) {
    uint32_t i;

    // Check if there already is a vertex that satisfies the requirements
    for (i = 0; i < odol_lod->num_points; i++) {
        if (odol_lod->vertex_to_point[i] != point_index_mlod)
            continue;

        // normals and uvs don't matter for non-visual lods
        if (mlod_lod->resolution < LOD_GRAPHICAL && (
                !float_equal(odol_lod->normals[i].x, normal->x, 0.001) ||
                !float_equal(odol_lod->normals[i].y, normal->y, 0.001) ||
                !float_equal(odol_lod->normals[i].z, normal->z, 0.001)))
            continue;

        if (mlod_lod->resolution < LOD_GRAPHICAL && (
                !float_equal(odol_lod->uv_coords[i].u, uv_coords->u, 0.001) ||
                !float_equal(odol_lod->uv_coords[i].v, uv_coords->v, 0.001)))
            continue;

        return i;
    }

    // Add vertex
    memcpy(&odol_lod->points[odol_lod->num_points], &mlod_lod->points[point_index_mlod], sizeof(struct triplet));
    memcpy(&odol_lod->normals[odol_lod->num_points], normal, sizeof(struct triplet));
    memcpy(&odol_lod->uv_coords[odol_lod->num_points], uv_coords, sizeof(struct uv_pair));

    odol_lod->vertex_to_point[odol_lod->num_points] = point_index_mlod;
    odol_lod->point_to_vertex[point_index_mlod] = odol_lod->num_points;

    odol_lod->num_points++;

    return (odol_lod->num_points - 1);
}


void convert_lod(struct mlod_lod *mlod_lod, struct odol_lod *odol_lod,
        struct model_info *model_info) {
    unsigned long i;
    unsigned long j;
    unsigned long k;
    unsigned long l;
    unsigned long face_end;
    bool first;
    bool sharp_edge;
    size_t size;
    char *ptr;
    char textures[MAXTEXTURES][512];
    struct triplet normal;
    struct uv_pair uv_coords;
    struct triplet v1;
    struct triplet v2;

    odol_lod->num_proxies = 0; // @todo
    odol_lod->num_items = 0; // @todo
    odol_lod->num_bonelinks = 0; // @todo

    odol_lod->num_points_mlod = mlod_lod->num_points;

    odol_lod->unknown_v52_float = 0;
    odol_lod->unknown_float_1 = 0;
    odol_lod->unknown_float_2 = 0;

    odol_lod->min_pos.x = 0;
    odol_lod->min_pos.y = 0;
    odol_lod->min_pos.z = 0;
    odol_lod->max_pos.x = 0;
    odol_lod->max_pos.y = 0;
    odol_lod->max_pos.z = 0;
    first = false;
    for (i = 0; i < mlod_lod->num_points; i++) {
        if (first || mlod_lod->points[i].x < odol_lod->min_pos.x)
            odol_lod->min_pos.x = mlod_lod->points[i].x;
        if (first || mlod_lod->points[i].y < odol_lod->min_pos.y)
            odol_lod->min_pos.y = mlod_lod->points[i].y;
        if (first || mlod_lod->points[i].z < odol_lod->min_pos.z)
            odol_lod->min_pos.z = mlod_lod->points[i].z;
        if (first || mlod_lod->points[i].x > odol_lod->max_pos.x)
            odol_lod->max_pos.x = mlod_lod->points[i].x;
        if (first || mlod_lod->points[i].y > odol_lod->max_pos.y)
            odol_lod->max_pos.y = mlod_lod->points[i].y;
        if (first || mlod_lod->points[i].z > odol_lod->max_pos.z)
            odol_lod->max_pos.z = mlod_lod->points[i].z;
    }

    odol_lod->autocenter_pos.x = (odol_lod->min_pos.x + odol_lod->max_pos.x) / 2;
    odol_lod->autocenter_pos.y = (odol_lod->min_pos.y + odol_lod->max_pos.y) / 2;
    odol_lod->autocenter_pos.z = (odol_lod->min_pos.z + odol_lod->max_pos.z) / 2;

    odol_lod->sphere = get_sphere(mlod_lod, &model_info->centre_of_mass);

    for (i = 0; i < MAXTEXTURES; i++)
        textures[i][0] = 0;

    odol_lod->num_textures = 0;
    size = 0;
    for (i = 0; i < mlod_lod->num_faces; i++) {
        for (j = 0; j < MAXTEXTURES && textures[j][0] != 0; j++) {
            if (strcmp(mlod_lod->faces[i].texture_name, textures[j]) == 0)
                break;
        }

        if (j >= MAXTEXTURES) {
            warningf("Maximum amount of textures per LOD (%i) exceeded.", MAXTEXTURES);
            break;
        }

        if (textures[j][0] != 0)
            continue;
        if (mlod_lod->faces[i].texture_name[0] == 0)
            continue;

        strcpy(textures[j], mlod_lod->faces[i].texture_name);
        odol_lod->num_textures++;
        size += strlen(textures[j]) + 1;
    }

    odol_lod->textures = (char *)malloc(size);
    ptr = odol_lod->textures;
    for (i = 0; i < MAXTEXTURES && textures[i][0] != 0; i++) {
        strncpy(ptr, textures[i], strlen(textures[i]) + 1);
        ptr += strlen(textures[i]) + 1;
    }

    odol_lod->num_materials = 0; // @todo;
    odol_lod->materials = 0;

    odol_lod->num_faces = mlod_lod->num_faces;
    odol_lod->offset_sections = sizeof(struct odol_face) * odol_lod->num_faces + 8;

    odol_lod->always_0 = 0;

    odol_lod->faces = (struct odol_face *)malloc(sizeof(struct odol_face) * odol_lod->num_faces);

    odol_lod->num_points = 0;

#ifdef VERSION70
    odol_lod->point_to_vertex = (uint32_t *)malloc(sizeof(uint32_t) * odol_lod->num_points_mlod);
    odol_lod->vertex_to_point = (uint32_t *)malloc(sizeof(uint32_t) * (odol_lod->num_faces * 4 + odol_lod->num_points_mlod));
#else
    odol_lod->point_to_vertex = (uint16_t *)malloc(sizeof(uint16_t) * odol_lod->num_points_mlod);
    odol_lod->vertex_to_point = (uint16_t *)malloc(sizeof(uint16_t) * (odol_lod->num_faces * 4 + odol_lod->num_points_mlod));
#endif

    for (i = 0; i < odol_lod->num_points_mlod; i++)
#ifdef VERSION70
        odol_lod->point_to_vertex[i] = UINT32_MAX;
#else
        odol_lod->point_to_vertex[i] = UINT16_MAX;
#endif

    odol_lod->uv_coords = (struct uv_pair *)malloc(sizeof(struct uv_pair) * (odol_lod->num_faces * 4 + odol_lod->num_points_mlod));
    odol_lod->points = (struct triplet *)malloc(sizeof(struct triplet) * (odol_lod->num_faces * 4 + odol_lod->num_points_mlod));
    odol_lod->normals = (struct triplet *)malloc(sizeof(struct triplet) * (odol_lod->num_faces * 4 + odol_lod->num_points_mlod));

    // Write face vertices
    face_end = 0;
    for (i = 0; i < mlod_lod->num_faces; i++) {
        odol_lod->faces[i].face_type = mlod_lod->faces[i].face_type;
        for (j = 0; j < odol_lod->faces[i].face_type; j++) {
            memcpy(&normal, &mlod_lod->facenormals[mlod_lod->faces[i].table[j].normals_index], sizeof(struct triplet));
            uv_coords.u = mlod_lod->faces[i].table[j].u;
            uv_coords.v = mlod_lod->faces[i].table[j].v;

            // Change vertex order for ODOL
            // Tris:  0 1 2   -> 1 0 2
            // Quads: 0 1 2 3 -> 1 0 3 2

            if (j <= 1)
                k = 1 - j;
            else if (odol_lod->faces[i].face_type == 3)
                k = j;
            else
                k = 2 + (1 - (j - 2));

            odol_lod->faces[i].table[k] = add_point(odol_lod, mlod_lod,
                mlod_lod->faces[i].table[j].points_index, &normal, &uv_coords);
        }
        face_end += 2 + 2 * odol_lod->faces[i].face_type;
    }

    odol_lod->offset_sections = face_end;

    // Write remaining vertices
    for (i = 0; i < odol_lod->num_points_mlod; i++) {
#ifdef VERSION70
        if (odol_lod->point_to_vertex[i] < UINT32_MAX)
#else
        if (odol_lod->point_to_vertex[i] < UINT16_MAX)
#endif
            continue;

        normal.x = 0.0f;
        normal.y = 0.0f;
        normal.z = 0.0f;

        uv_coords.u = 0.0f;
        uv_coords.v = 0.0f;

        odol_lod->point_to_vertex[i] = add_point(odol_lod, mlod_lod,
            i, &normal, &uv_coords);
    }

    if (odol_lod->num_faces > 0) {
        odol_lod->num_sections = 1; // @todo
        odol_lod->sections = (struct odol_section *)malloc(sizeof(struct odol_section) * odol_lod->num_sections);

        odol_lod->sections[0].face_index_start = 0;
        odol_lod->sections[0].face_index_end = face_end;
        odol_lod->sections[0].material_index_start = 0;
        odol_lod->sections[0].material_index_end = 0;
        odol_lod->sections[0].common_point_flags = 0;
        odol_lod->sections[0].common_texture_index = (odol_lod->num_textures == 1) ? 0 : -1;
        odol_lod->sections[0].common_face_flags = 0;
        odol_lod->sections[0].material_index = -1;
        odol_lod->sections[0].unknown_long_1 = 2;
        odol_lod->sections[0].unknown_resolution_1 = 0.0f;
        odol_lod->sections[0].unknown_resolution_2 = 1000.0f;
        odol_lod->sections[0].unknown_long_2 = 0;
    } else {
        odol_lod->num_sections = 0;
        odol_lod->sections = 0;
    }

    odol_lod->num_selections = mlod_lod->num_selections;
    odol_lod->selections = (struct odol_selection *)malloc(sizeof(struct odol_selection) * odol_lod->num_selections);

    for (i = 0; i < odol_lod->num_selections; i++) {
        strcpy(odol_lod->selections[i].name, mlod_lod->selections[i].name);
        lower_case(odol_lod->selections[i].name);

        odol_lod->selections[i].num_faces = 0;
        for (j = 0; j < odol_lod->num_faces; j++) {
            if (mlod_lod->selections[i].faces[j] > 0)
                odol_lod->selections[i].num_faces++;
        }

#ifdef VERSION70
        odol_lod->selections[i].faces = (uint32_t *)malloc(sizeof(uint32_t) * odol_lod->selections[i].num_faces);
        for (j = 0; j < odol_lod->selections[i].num_faces; j++)
            odol_lod->selections[i].faces[j] = UINT32_MAX;
#else
        odol_lod->selections[i].faces = (uint16_t *)malloc(sizeof(uint16_t) * odol_lod->selections[i].num_faces);
        for (j = 0; j < odol_lod->selections[i].num_faces; j++)
            odol_lod->selections[i].faces[j] = UINT16_MAX;
#endif

        for (j = 0; j < odol_lod->num_faces; j++) {
            if (mlod_lod->selections[i].faces[j] == 0)
                continue;

            for (k = 0; k < odol_lod->selections[i].num_faces; k++) {
#ifdef VERSION70
                if (odol_lod->selections[i].faces[k] == UINT16_MAX)
                    break;
#else
                if (odol_lod->selections[i].faces[k] == UINT16_MAX)
                    break;
#endif
            }
            odol_lod->selections[i].faces[k] = j;
        }

        odol_lod->selections[i].always_0 = 0;
        odol_lod->selections[i].is_sectional = false;
        odol_lod->selections[i].num_sections = 0;
        odol_lod->selections[i].sections = 0;

        odol_lod->selections[i].num_vertices = 0;
        for (j = 0; j < odol_lod->num_points; j++) {
            if (mlod_lod->selections[i].points[odol_lod->vertex_to_point[j]] > 0)
                odol_lod->selections[i].num_vertices++;
        }

        odol_lod->selections[i].num_vertex_weights = odol_lod->selections[i].num_vertices;

#ifdef VERSION70
        odol_lod->selections[i].vertices = (uint32_t *)malloc(sizeof(uint32_t) * odol_lod->selections[i].num_vertices);
        for (j = 0; j < odol_lod->selections[i].num_vertices; j++)
            odol_lod->selections[i].vertices[j] = UINT32_MAX;
#else
        odol_lod->selections[i].vertices = (uint16_t *)malloc(sizeof(uint16_t) * odol_lod->selections[i].num_vertices);
        for (j = 0; j < odol_lod->selections[i].num_vertices; j++)
            odol_lod->selections[i].vertices[j] = UINT16_MAX;
#endif
        odol_lod->selections[i].vertex_weights = (uint8_t *)malloc(sizeof(uint8_t) * odol_lod->selections[i].num_vertex_weights);

        for (j = 0; j < odol_lod->num_points; j++) {
            if (mlod_lod->selections[i].points[odol_lod->vertex_to_point[j]] == 0)
                continue;

            for (k = 0; k < odol_lod->selections[i].num_vertices; k++) {
#ifdef VERSION70
                if (odol_lod->selections[i].vertices[k] == UINT16_MAX)
                    break;
#else
                if (odol_lod->selections[i].vertices[k] == UINT16_MAX)
                    break;
#endif
            }
            odol_lod->selections[i].vertices[k] = j;
            odol_lod->selections[i].vertex_weights[k] = mlod_lod->selections[i].points[odol_lod->vertex_to_point[j]];
        }
    }

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

    odol_lod->unknown_residue = 0;
    odol_lod->unknown_byte = 0;

    odol_lod->uv_scale[0] = 0;
    odol_lod->uv_scale[1] = 0;
    odol_lod->uv_scale[2] = 1;
    odol_lod->uv_scale[3] = 1;
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
    fwrite(&model_info->mem_lod_sphere,      sizeof(float), 1, f_target);
    fwrite(&model_info->geo_lod_sphere,      sizeof(float), 1, f_target);
    fwrite( model_info->point_flags,         sizeof(uint32_t) * 3, 1, f_target);
    fwrite(&model_info->aiming_center,       sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->map_icon_color,      sizeof(uint32_t), 1, f_target);
    fwrite(&model_info->map_selected_color,  sizeof(uint32_t), 1, f_target);
    fwrite(&model_info->view_density,        sizeof(float), 1, f_target);
    fwrite(&model_info->bbox_min,            sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->bbox_max,            sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->bbox_visual_min,     sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->bbox_visual_max,     sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->bounding_center,     sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->geometry_center,     sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->centre_of_mass,      sizeof(struct triplet), 1, f_target);
    fwrite(&model_info->inv_inertia,         sizeof(struct triplet), 1, f_target);
    fwrite( model_info->thermal_profile2,    sizeof(char) * 24, 1, f_target);
    fwrite(&model_info->autocenter,          sizeof(bool), 1, f_target);
    fwrite(&model_info->lock_autocenter,     sizeof(bool), 1, f_target);
    fwrite(&model_info->can_occlude,         sizeof(bool), 1, f_target);
    fwrite(&model_info->can_be_occluded,     sizeof(bool), 1, f_target);
    fwrite(&model_info->allow_animation,     sizeof(bool), 1, f_target);
    fwrite( model_info->unknown_flags,       sizeof(char) * 6, 1, f_target);
    fwrite( model_info->thermal_profile,     sizeof(char) * 24, 1, f_target);
    fwrite(&model_info->unknown_long,        sizeof(uint32_t), 1, f_target);
    write_skeleton(f_target, model_info->skeleton);
    fwrite(&model_info->unknown_byte,        sizeof(char), 1, f_target);
    fwrite(&model_info->n_floats,            sizeof(uint32_t), 1, f_target);
    //fwrite("\0\0\0\0\0", 4, 1, f_target); // compression header for empty array
    fwrite(&model_info->mass,                sizeof(float), 1, f_target);
    fwrite(&model_info->mass_reciprocal,     sizeof(float), 1, f_target);
    fwrite(&model_info->alt_mass,            sizeof(float), 1, f_target);
    fwrite(&model_info->alt_mass_reciprocal, sizeof(float), 1, f_target);
    fwrite( model_info->unknown_indices,     sizeof(char) * 14, 1, f_target);
    fwrite(&model_info->unknown_long_2,      sizeof(uint32_t), 1, f_target);
    fwrite(&model_info->unknown_bool,        sizeof(bool), 1, f_target);
    fwrite(&model_info->class_type,          sizeof(char), 1, f_target);
    fwrite(&model_info->destruct_type,       sizeof(char), 1, f_target);
    fwrite(&model_info->unknown_bool_2,      sizeof(bool), 1, f_target);
    fwrite(&model_info->always_0,            sizeof(uint32_t), 1, f_target);

    for (i = 0; i < num_lods; i++)
        fwrite("\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff", 12, 1, f_target);
}


void write_odol_section(FILE *f_target, struct odol_section *odol_section) {
    fwrite(&odol_section->face_index_start,     sizeof(uint32_t), 1, f_target);
    fwrite(&odol_section->face_index_end,       sizeof(uint32_t), 1, f_target);
    fwrite(&odol_section->material_index_start, sizeof(uint32_t), 1, f_target);
    fwrite(&odol_section->material_index_end,   sizeof(uint32_t), 1, f_target);
    fwrite(&odol_section->common_point_flags,   sizeof(uint32_t), 1, f_target);
    fwrite(&odol_section->common_texture_index, sizeof(uint16_t), 1, f_target);
    fwrite(&odol_section->common_face_flags,    sizeof(uint32_t), 1, f_target);
    fwrite(&odol_section->material_index,       sizeof(int32_t), 1, f_target);
    if (odol_section->material_index == -1)
        fputc(0, f_target);
    fwrite(&odol_section->unknown_long_1,       sizeof(uint32_t), 1, f_target);
    fwrite(&odol_section->unknown_resolution_1, sizeof(float), 1, f_target);
    fwrite(&odol_section->unknown_resolution_2, sizeof(float), 1, f_target);
    fwrite(&odol_section->unknown_long_2,       sizeof(uint32_t), 1, f_target);
}


void write_odol_selection(FILE *f_target, struct odol_selection *odol_selection) {
    fwrite(odol_selection->name, strlen(odol_selection->name) + 1, 1, f_target);

    fwrite(&odol_selection->num_faces, sizeof(uint32_t), 1, f_target);
    if (odol_selection->num_faces > 0) {
        fputc(0, f_target);
#ifdef VERSION70
        fwrite(odol_selection->faces, sizeof(uint32_t) * odol_selection->num_faces, 1, f_target);
#else
        fwrite(odol_selection->faces, sizeof(uint16_t) * odol_selection->num_faces, 1, f_target);
#endif
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
#ifdef VERSION70
        fwrite(odol_selection->vertices, sizeof(uint32_t) * odol_selection->num_vertices, 1, f_target);
#else
        fwrite(odol_selection->vertices, sizeof(uint16_t) * odol_selection->num_vertices, 1, f_target);
#endif
    }

    odol_selection->num_vertex_weights = 0;
    fwrite(&odol_selection->num_vertex_weights, sizeof(uint32_t), 1, f_target);
    return;
    if (odol_selection->num_vertex_weights > 0) {
        fputc(0, f_target);
        fwrite(odol_selection->vertex_weights, sizeof(uint8_t) * odol_selection->num_vertex_weights, 1, f_target);
    }
}


void write_odol_lod(FILE *f_target, struct odol_lod *odol_lod) {
    short u, v;
    int x, y, z;
    long i;
    long fp_vertextable_size;
    uint32_t temp;
    char *ptr;

    fwrite(&odol_lod->num_proxies, sizeof(uint32_t), 1, f_target);
    // @todo proxies
    fwrite(&odol_lod->num_items, sizeof(uint32_t), 1, f_target);
    fwrite( odol_lod->items, sizeof(uint32_t) * odol_lod->num_items, 1, f_target);
    fwrite(&odol_lod->num_bonelinks, sizeof(uint32_t), 1, f_target);
    // @todo bonelinks
    fwrite(&odol_lod->num_points, sizeof(uint32_t), 1, f_target);
    fwrite(&odol_lod->unknown_v52_float, sizeof(float), 1, f_target);
    fwrite(&odol_lod->unknown_float_1, sizeof(float), 1, f_target);
    fwrite(&odol_lod->unknown_float_2, sizeof(float), 1, f_target);
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
    // @todo materials

    // the point-to-vertex and vertex-to-point arrays are just left out
    fwrite("\0\0\0\0\0\0\0\0", sizeof(uint32_t) * 2, 1, f_target);

    fwrite(&odol_lod->num_faces, sizeof(uint32_t), 1, f_target);
    fwrite(&odol_lod->offset_sections, sizeof(uint32_t), 1, f_target);
    fwrite(&odol_lod->always_0, sizeof(uint16_t), 1, f_target);

    for (i = 0; i < odol_lod->num_faces; i++) {
        fwrite(&odol_lod->faces[i].face_type, sizeof(uint8_t), 1, f_target);
#ifdef VERSION70
        fwrite( odol_lod->faces[i].table, sizeof(uint32_t) * odol_lod->faces[i].face_type, 1, f_target);
#else
        fwrite( odol_lod->faces[i].table, sizeof(uint16_t) * odol_lod->faces[i].face_type, 1, f_target);
#endif
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
    fwrite(&odol_lod->unknown_residue, sizeof(uint32_t), 1, f_target);
    fwrite(&odol_lod->unknown_byte, sizeof(char), 1, f_target);

    fp_vertextable_size = ftell(f_target);
    fwrite("\0\0\0\0", 4, 1, f_target);

    // pointflags
    fwrite(&odol_lod->num_points, 4, 1, f_target);
    fwrite("\x01\0\0\0\0", 5, 1, f_target);

    // uvs
    fwrite( odol_lod->uv_scale, sizeof(float) * 4, 1, f_target);
    fwrite(&odol_lod->num_points, sizeof(uint32_t), 1, f_target);
    fputc(0, f_target);
    fputc(0, f_target);
    for (i = 0; i < odol_lod->num_points; i++) {
        // write compressed pair
        u = (short)(odol_lod->uv_coords[i].u * 2 * INT16_MAX - INT16_MAX);
        v = (short)(odol_lod->uv_coords[i].v * 2 * INT16_MAX - INT16_MAX);

        fwrite(&u, sizeof(int16_t), 1, f_target);
        fwrite(&v, sizeof(int16_t), 1, f_target);
    }
    fwrite("\x01\0\0\0", 4, 1, f_target);

    // points
    fwrite(&odol_lod->num_points, sizeof(uint32_t), 1, f_target);
    fputc(0, f_target);
    fwrite( odol_lod->points, sizeof(struct triplet) * odol_lod->num_points, 1, f_target);

    // normals
    fwrite(&odol_lod->num_points, sizeof(uint32_t), 1, f_target);
    fputc(0, f_target);
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

    fwrite("\0\0\0\0\0\0\0\0\0\0\0\0", 12, 1, f_target);

    temp = ftell(f_target) - fp_vertextable_size;
    fseek(f_target, fp_vertextable_size, SEEK_SET);
    fwrite(&temp, 4, 1, f_target);
    fseek(f_target, 0, SEEK_END);
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

    anim->axis_pos.x = 0;
    anim->axis_pos.y = 0;
    anim->axis_pos.z = 0;
    anim->axis_dir.x = 0;
    anim->axis_dir.y = 0;
    anim->axis_dir.z = 0;

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
                        memcpy(&anim->axis_pos, &mlod_lods[i].points[k], sizeof(struct triplet));
                        break;
                    }
                }
            }
        } else {
            if (stricmp(mlod_lods[i].selections[j].name, anim->axis) == 0) {
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
    }

    anim->axis_dir.x -= anim->axis_pos.x;
    anim->axis_dir.y -= anim->axis_pos.y;
    anim->axis_dir.z -= anim->axis_pos.z;
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
        fwrite(&anim->min_phase, sizeof(float), 1, f_target);
        fwrite(&anim->max_phase, sizeof(float), 1, f_target);
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
                if (strcmp(anim->selection, model_info->skeleton->bones[j].name) == 0)
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

            if (index == -1) {
                if (i == 0) {
                    warningf("Failed to find bone \"%s\" for animation \"%s\".\n",
                            model_info->skeleton->bones[k].name, anim->name);
                }
            } else {
                if (anim->type >= 8)
                    continue;

                calculate_axis(anim, num_lods, mlod_lods);

                anim->axis_pos.x -= model_info->centre_of_mass.x;
                anim->axis_pos.y -= model_info->centre_of_mass.y;
                anim->axis_pos.z -= model_info->centre_of_mass.z;

                fwrite(&anim->axis_pos, sizeof(struct triplet), 1, f_target);
                fwrite(&anim->axis_dir, sizeof(struct triplet), 1, f_target);
            }
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
    uint32_t num_lods;
    struct mlod_lod *mlod_lods;
    struct model_info model_info;
    struct odol_lod odol_lod;

#ifdef _WIN32
    char temp_name[2048];
    if (!GetTempFileName(getenv("HOMEPATH"), "amk", 0, temp_name)) {
        errorf("Failed to get temp file name.\n");
        return 1;
    }
    f_temp = fopen(temp_name, "w+");
#else
    f_temp = tmpfile();
#endif

    if (!f_temp) {
        errorf("Failed to open temp file.\n");
        return 1;
    }

    // Open source and read LODs
    f_source = fopen(source, "r");
    if (!f_source) {
        errorf("Failed to open source file.\n");
        return 2;
    }

    fgets(buffer, 5, f_source);
    if (strncmp(buffer, "MLOD", 4) != 0) {
        errorf("Source file is not MLOD.\n");
        return -3;
    }

    fseek(f_source, 8, SEEK_SET);
    fread(&num_lods, 4, 1, f_source);
    mlod_lods = (struct mlod_lod *)malloc(sizeof(struct mlod_lod) * num_lods);
    if (read_lods(f_source, mlod_lods, num_lods)) {
        errorf("Failed to read LODs.\n");
        return 4;
    }

    fclose(f_source);

    // Write header
    fwrite("ODOL", 4, 1, f_temp);
#ifdef VERSION70
    fwrite("\x46\0\0\0", 4, 1, f_temp); // version 70
#else
    fwrite("\x44\0\0\0", 4, 1, f_temp); // version 68
#endif
    // there seem to be another 4 bytes here, no idea what for
    fwrite("\0\0\0\0", 4, 1, f_temp);
    fwrite("\0", 1, 1, f_temp); // prefix
    fwrite(&num_lods, 4, 1, f_temp);

    // Write model info
    build_model_info(mlod_lods, num_lods, &model_info);
    success = read_model_config(source, model_info.skeleton);
    if (success > 0)
        return success;

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
        free(odol_lod.textures);
        free(odol_lod.point_to_vertex);
        free(odol_lod.vertex_to_point);
        free(odol_lod.faces);
        free(odol_lod.uv_coords);
        free(odol_lod.points);
        free(odol_lod.normals);
        free(odol_lod.sections);

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

    f_target = fopen(target, "w");
    if (!f_target) {
        errorf("Failed to open target file.\n");
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
