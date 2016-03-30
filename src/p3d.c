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
#include "p3d.h"


bool float_equal(float f1, float f2) {
    return fabs(1.0 - (f1 / f2)) < 0.01;
}


int read_lods(FILE *f_source, struct mlod_lod *mlod_lods, uint32_t num_lods) {
    char buffer[256];
    int i;
    int j;
    int fp_tmp;
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
        for (j = 0; j < mlod_lods[i].num_faces; j++) {
            fread(&mlod_lods[i].faces[j], 72, 1, f_source);

            fp_tmp = ftell(f_source);
            fread(&mlod_lods[i].faces[j].texture_name,
                sizeof(mlod_lods[i].faces[j].texture_name),
                1,
                f_source);
            fseek(f_source, fp_tmp + strlen(mlod_lods[i].faces[j].texture_name) + 1, SEEK_SET);

            fp_tmp = ftell(f_source);
            fread(&mlod_lods[i].faces[j].material_name,
                sizeof(mlod_lods[i].faces[j].material_name),
                1,
                f_source);
            fseek(f_source, fp_tmp + strlen(mlod_lods[i].faces[j].material_name) + 1, SEEK_SET);
        }

        fread(buffer, 4, 1, f_source);
        if (strncmp(buffer, "TAGG", 4) != 0)
            return 2;

        mlod_lods[i].mass = 0;

        while (true) {
            fseek(f_source, 1, SEEK_CUR);

            fp_tmp = ftell(f_source);
            fread(&buffer, sizeof(buffer), 1, f_source);
            fseek(f_source, fp_tmp + strlen(buffer) + 1, SEEK_SET);

            fread(&tagg_len, 4, 1, f_source);
            fp_tmp = ftell(f_source) + tagg_len;

            if (strcmp(buffer, "#Mass#") == 0) {
                mlod_lods[i].mass = (float *)malloc(sizeof(float) * mlod_lods[i].num_points);
                fread(mlod_lods[i].mass, sizeof(float) * mlod_lods[i].num_points, 1, f_source);
            }

            // @todo TAGGs: property, mass, animation, uvset

            fseek(f_source, fp_tmp, SEEK_SET);

            if (strcmp(buffer, "#EndOfFile#") == 0)
                break;
        }

        fread(&mlod_lods[i].resolution, 4, 1, f_source);
    }

    return 0;
}


void get_centre_of_gravity(struct mlod_lod *mlod_lods, uint32_t num_lods, struct triplet *centre_of_gravity) {
    int i;
    int j;
    float mass;
    float x;
    float y;
    float z;

    centre_of_gravity->x = 0;
    centre_of_gravity->y = 0;
    centre_of_gravity->z = 0;

    for (i = 0; i < num_lods; i++) {
        if (float_equal(mlod_lods[i].resolution, GEOMETRY))
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

        centre_of_gravity->x += x * (mlod_lods[i].mass[j] / mass);
        centre_of_gravity->y += y * (mlod_lods[i].mass[j] / mass);
        centre_of_gravity->z += z * (mlod_lods[i].mass[j] / mass);
    }
}


void get_bounding_box(struct mlod_lod *mlod_lods, uint32_t num_lods, struct triplet *bbox_min, struct triplet *bbox_max) {
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


int build_model_info(struct mlod_lod *mlod_lods, uint32_t num_lods, struct model_info *model_info) {
    int i;
    int j;
    struct triplet bbox_total_min;
    struct triplet bbox_total_max;

    model_info->lod_resolutions = (float *)malloc(sizeof(float) * num_lods);

    for (i = 0; i < num_lods; i++)
        model_info->lod_resolutions[i] = mlod_lods[i].resolution;

    model_info->index = 512; // @todo
    model_info->mem_lod_sphere = 0; // @todo
    model_info->geo_lod_sphere = 0; // @todo

    model_info->point_flags[0] = 0; // @todo
    model_info->point_flags[1] = 0; // @todo
    model_info->point_flags[2] = 0; // @todo

    model_info->map_icon_color = 0; // @todo
    model_info->map_selected_color = 0; // @todo

    model_info->view_density = -1;

    get_centre_of_gravity(mlod_lods, num_lods, &model_info->centre_of_gravity);
    get_bounding_box(mlod_lods, num_lods, &bbox_total_min, &bbox_total_max);

    bbox_total_min.x -= model_info->centre_of_gravity.x;
    bbox_total_min.y -= model_info->centre_of_gravity.y;
    bbox_total_min.z -= model_info->centre_of_gravity.z;
    bbox_total_max.x -= model_info->centre_of_gravity.x;
    bbox_total_max.y -= model_info->centre_of_gravity.y;
    bbox_total_max.z -= model_info->centre_of_gravity.z;

    model_info->bbox_max.x = fabs(bbox_total_max.x - bbox_total_min.x) / 2;
    model_info->bbox_max.y = fabs(bbox_total_max.y - bbox_total_min.y) / 2;
    model_info->bbox_max.z = fabs(bbox_total_max.z - bbox_total_min.z) / 2;
    model_info->bbox_min.x = model_info->bbox_max.x * -1;
    model_info->bbox_min.y = model_info->bbox_max.y * -1;
    model_info->bbox_min.z = model_info->bbox_max.z * -1;

    model_info->cog_offset.x = model_info->bbox_max.x - bbox_total_max.x + model_info->centre_of_gravity.x;
    model_info->cog_offset.y = model_info->bbox_max.y - bbox_total_max.y + model_info->centre_of_gravity.y;
    model_info->cog_offset.z = model_info->bbox_max.z - bbox_total_max.z + model_info->centre_of_gravity.z;

    model_info->centre_of_gravity.x -= model_info->cog_offset.x;
    model_info->centre_of_gravity.y -= model_info->cog_offset.y;
    model_info->centre_of_gravity.z -= model_info->cog_offset.z;

    memcpy(&model_info->centre_of_gravity, &model_info->offset1, sizeof(struct triplet));
    memcpy(&model_info->centre_of_gravity, &model_info->offset2, sizeof(struct triplet));

    model_info->model_mass_vectors[0].x = 0; // @todo
    model_info->model_mass_vectors[0].y = 0;
    model_info->model_mass_vectors[0].z = 0;
    model_info->model_mass_vectors[1].x = 0;
    model_info->model_mass_vectors[1].y = 0;
    model_info->model_mass_vectors[1].z = 0;
    model_info->model_mass_vectors[2].x = 0;
    model_info->model_mass_vectors[2].y = 0;
    model_info->model_mass_vectors[2].z = 0;

    strncpy(model_info->thermal_profile2,
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
        sizeof(model_info->thermal_profile2)); // @todo

    model_info->autocenter = false; // @todo
    model_info->lock_autocenter = false; // @todo
    model_info->can_occlude = false; // @todo
    model_info->can_be_occluded = false; // @todo
    model_info->allow_animation = false; // @todo

    strncpy(model_info->unknown_flags, "\0\0\0\0\0\0", sizeof(model_info->unknown_flags)); // @todo
    strncpy(model_info->thermal_profile,
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
        sizeof(model_info->thermal_profile)); // @todo

    model_info->unknown_long = 0;

    // @todo: skeleton

    model_info->unknown_byte = 0;
    model_info->n_floats = 0;

    model_info->mass = 0;
    for (i = 0; i < num_lods; i++) {
        if (!float_equal(mlod_lods[i].resolution, GEOMETRY))
            continue;
        for (j = 0; j < mlod_lods[i].num_points; j++)
            model_info->mass += mlod_lods[i].mass[j];
    }
    model_info->mass_reciprocal = model_info->mass; // @todo
    model_info->alt_mass = model_info->mass; // @todo
    model_info->alt_mass_reciprocal = model_info->mass; // @todo

    strncpy(model_info->unknown_indices,
        "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff",
        sizeof(model_info->unknown_indices));

    model_info->unknown_long_2 = num_lods;
    model_info->unknown_bool = false;
    model_info->class_type[0] = 0;
    model_info->destruct_type[0] = 0;
    model_info->unknown_bool_2 = false;
    model_info->always_0 = 0;

    return 0;
}


int write_model_info(FILE *f_target, uint32_t num_lods, struct model_info *model_info) {
    int i;

    fwrite(model_info->lod_resolutions, sizeof(float) * num_lods, 1, f_target);

    fwrite(model_info + sizeof(model_info->lod_resolutions), sizeof(model_info) - sizeof(model_info->lod_resolutions), 1, f_target);

    for (i = 0; i < num_lods; i++)
        fwrite("\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff", 12, 1, f_target);

    return 0;
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
    uint32_t num_lods;
    struct mlod_lod *mlod_lods;
    struct model_info model_info;

#ifdef _WIN32
    char temp_name[2048];
    if (!GetTempFileName(getenv("HOMEPATH"), "amk", 0, temp_name)) {
        printf("Failed to get temp file name.\n");
        return 1;
    }
    f_temp = fopen(temp_name, "w+");
#else
    f_temp = tmpfile();
#endif

    if (!f_temp) {
        printf("Failed to open temp file.\n");
        return 1;
    }

    // Open source and read LODs
    f_source = fopen(source, "r");
    if (!f_source) {
        printf("Failed to open source file.\n");
        return 2;
    }

    fgets(buffer, sizeof(buffer), f_source);
    if (strncmp(buffer, "MLOD", 4) != 0) {
        printf("Source file is not MLOD.\n");
        return 3;
    }

    fseek(f_source, 8, SEEK_SET);
    fread(&num_lods, 4, 1, f_source);
    mlod_lods = (struct mlod_lod *)malloc(sizeof(struct mlod_lod) * num_lods);
    if (read_lods(f_source, mlod_lods, num_lods)) {
        printf("Failed to read LODs.\n");
        return 4;
    }

    //for (i = 0; i < num_lods; i++) {
    //    printf("LOD %i: %f %i %i %i\n", i, mlod_lods[i].resolution,
    //        mlod_lods[i].num_points,
    //        mlod_lods[i].num_facenormals,
    //        mlod_lods[i].num_faces);
    //}

    //fclose(f_source);

    // Write header
    fwrite("ODOL", 4, 1, f_temp);
    fwrite("\x46\0\0\0", 4, 1, f_temp); // version 70
    fwrite("\0", 1, 1, f_temp); // prefix
    fwrite(&num_lods, 4, 1, f_temp);

    // Write model info
    if (build_model_info(mlod_lods, num_lods, &model_info)) {
        printf("Failed to construct model info.\n");
        return 5;
    }
    return 0;

    // Write animations (@todo)
    fwrite("\0", 1, 1, f_temp); // nope

    // Write LODs (@todo)

    // Write PhysX (@todo)
    //fwrite("\0\0\0\0", 4, 1, f_target);

    // Write temp to target
    fseek(f_temp, 0, SEEK_END);
    datasize = ftell(f_temp);

    f_target = fopen(target, "w");
    if (!f_target) {
        printf("Failed to open target file.\n");
        return 2;
    }

    fseek(f_temp, 0, SEEK_SET);
    for (i = 0; datasize - i >= sizeof(buffer); i += sizeof(buffer)) {
        fread(buffer, sizeof(buffer), 1, f_temp);
        fwrite(buffer, sizeof(buffer), 1, f_target);
    }
    fread(buffer, datasize - i, 1, f_temp);
    fwrite(buffer, datasize - i, 1, f_target);

    fclose(f_temp);
    fclose(f_target);

#ifdef _WIN32
    DeleteFile(temp_name);
#endif

    return 0;
}
