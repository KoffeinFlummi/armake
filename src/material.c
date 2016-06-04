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

#include "docopt.h"
#include "filesystem.h"
#include "rapify.h"
#include "preprocess.h"
#include "utils.h"
#include "derapify.h"
#include "material.h"


const struct shader_ref pixelshaders[30] = {
    { 0, "Normal" },
    { 1, "NormalDXTA" },
    { 2, "NormalMap" },
    { 3, "NormalMapThrough" },
    { 4, "NormalMapSpecularDIMap" },
    { 5, "NormalMapDiffuse" },
    { 6, "Detail" },
    { 8, "Water" },
    { 12, "AlphaShadow" },
    { 13, "AlphaNoShadow" },
    { 15, "DetailMacroAS" },
    { 18, "NormalMapSpecularMap" },
    { 19, "NormalMapDetailSpecularMap" },
    { 20, "NormalMapMacroASSpecularMap" },
    { 21, "NormalMapDetailMacroASSpecularMap" },
    { 22, "NormalMapSpecularDIMap" },
    { 23, "NormalMapDetailSpecularDIMap" },
    { 24, "NormalMapMacroASSpecularDIMap" },
    { 25, "NormalMapDetailMacroASSpecularDIMap" },
    { 56, "Glass" },
    { 58, "NormalMapSpecularThrough" },
    { 56, "Grass" },
    { 60, "NormalMapThroughSimple" },
    { 102, "Super" },
    { 103, "Multi" },
    { 107, "Tree" },
    { 110, "Skin" },
    { 111, "CalmWater" },
    { 114, "TreeAdv" },
    { 116, "TreeAdvTrunk" }
};

const struct shader_ref vertexshaders[20] = {
    { 0, "Basic" },
    { 1, "NormalMap" },
    { 2, "NormalMapDiffuse" },
    { 3, "Grass" },
    { 8, "Water" },
    { 11, "NormalMapThrough" },
    { 14, "BasicAS" },
    { 15, "NormalMapAS" },
    { 17, "Glass" },
    { 18, "NormalMapSpecularThrough" },
    { 19, "NormalMapThroughNoFade" },
    { 20, "NormalMapSpecularThroughNoFade" },
    { 23, "Super" },
    { 24, "Multi" },
    { 25, "Tree" },
    { 26, "TreeNoFade" },
    { 29, "Skin" },
    { 30, "CalmWater" },
    { 31, "TreeAdv" },
    { 32, "TreeAdvTrunk" }
};


int read_material(struct material *material) {
    /*
     * Reads the material information for the given material struct.
     * Returns 0 on success and a positive integer on failure.
     */

    extern int current_operation;
    extern char current_target[2048];
    FILE *f;
    char actual_path[2048];
    char rapified_path[2048];
    char config_path[2048];
    char temp[2048];
    char shader[2048];
    int i;
    struct color default_color = { 0.0f, 0.0f, 0.0f, 1.0f };

    if (material->path[0] != '\\') {
        strcpy(temp, "\\");
        strcat(temp, material->path);
    } else {
        strcpy(temp, material->path);
    }

    current_operation = OP_MATERIAL;
    strcpy(current_target, temp);

    if (find_file(temp, "", actual_path, NULL)) {
        errorf("Failed to find material %s.\n", material->path);
        return 1;
    }

    strcpy(rapified_path, actual_path);
    strcat(rapified_path, ".armake.bin"); // it is assumed that this doesn't exist

    // Rapify file
    if (rapify_file(actual_path, rapified_path)) {
        errorf("Failed to rapify %s.\n", actual_path);
        return 2;
    }

    current_operation = OP_MATERIAL;
    strcpy(current_target, material->path);

    // Open rapified file
    f = fopen(rapified_path, "rb");
    if (!f) {
        errorf("Failed to open rapified material.\n");
        return 3;
    }

    material->type = MATERIALTYPE;
    material->depr_1 = 1;
    material->depr_2 = 1;
    material->depr_3 = 1;

    // Read colors
    material->emissive = default_color;
    read_float_array(f, "emmisive", (float *)&material->emissive, 4); // "Did you mean: emissive?"

    material->ambient = default_color;
    read_float_array(f, "ambient", (float *)&material->ambient, 4);

    material->diffuse = default_color;
    read_float_array(f, "diffuse", (float *)&material->diffuse, 4);

    material->forced_diffuse = default_color;
    read_float_array(f, "forcedDiffuse", (float *)&material->forced_diffuse, 4);

    material->specular = default_color;
    read_float_array(f, "specular", (float *)&material->specular, 4);
    material->specular2 = material->specular;

    material->specular_power = 1.0f;
    read_float(f, "specularPower", &material->specular_power);

    // Read shaders
    material->pixelshader_id = 0;
    if (!read_string(f, "PixelShaderID", shader, sizeof(shader))) {
        for (i = 0; i < sizeof(pixelshaders) / sizeof(struct shader_ref); i++) {
            if (stricmp((char *)pixelshaders[i].name, shader) == 0)
                break;
        }
        if (i == sizeof(pixelshaders) / sizeof(struct shader_ref)) {
            warningf("Unrecognized pixel shader: \"%s\", assuming \"Normal\".\n", shader);
            i = 0;
        }
        material->pixelshader_id = pixelshaders[i].id;
    }

    material->vertexshader_id = 0;
    if (!read_string(f, "VertexShaderID", shader, sizeof(shader))) {
        for (i = 0; i < sizeof(vertexshaders) / sizeof(struct shader_ref); i++) {
            if (stricmp((char *)vertexshaders[i].name, shader) == 0)
                break;
        }
        if (i == sizeof(vertexshaders) / sizeof(struct shader_ref)) {
            warningf("Unrecognized vertex shader: \"%s\", assuming \"Basic\".\n", shader);
            i = 0;
        }
        material->vertexshader_id = vertexshaders[i].id;
    }

    // Read stages
    material->num_textures = 1;
    material->num_transforms = 1;
    for (i = 1; i < MAXSTAGES; i++) {
        snprintf(config_path, sizeof(config_path), "Stage%i >> texture", i);
        if (read_string(f, config_path, temp, sizeof(temp)))
            break;
        material->num_textures++;
        material->num_transforms++;
    }

    material->textures = (struct stage_texture *)malloc(sizeof(struct stage_texture) * material->num_textures);
    material->transforms = (struct stage_transform *)malloc(sizeof(struct stage_transform) * material->num_transforms);

    for (i = 0; i < material->num_textures; i++) {
        if (i == 0) {
            material->textures[i].path[0] = 0;
        } else {
            snprintf(config_path, sizeof(config_path), "Stage%i >> texture", i);
            read_string(f, config_path, material->textures[i].path, sizeof(material->textures[i].path));
        }

        material->textures[i].texture_filter = 3;
        material->textures[i].transform_index = i;
        material->textures[i].type11_bool = 0;

        material->transforms[i].uv_source = 1;
        material->transforms[i].transform[0][0] = 1.0f;
        material->transforms[i].transform[0][1] = 0.0f;
        material->transforms[i].transform[0][2] = 0.0f;
        material->transforms[i].transform[1][0] = 0.0f;
        material->transforms[i].transform[1][1] = 1.0f;
        material->transforms[i].transform[1][2] = 0.0f;
        material->transforms[i].transform[2][0] = 0.0f;
        material->transforms[i].transform[2][1] = 0.0f;
        material->transforms[i].transform[2][2] = 1.0f;
        material->transforms[i].transform[3][0] = 0.0f;
        material->transforms[i].transform[3][1] = 0.0f;
        material->transforms[i].transform[3][2] = 0.0f;

        if (i != 0) {
            snprintf(config_path, sizeof(config_path), "Stage%i >> uvTransform >> aside", i + 1);
            read_float_array(f, config_path, material->transforms[i].transform[0], 4);

            snprintf(config_path, sizeof(config_path), "Stage%i >> uvTransform >> up", i + 1);
            read_float_array(f, config_path, material->transforms[i].transform[1], 4);

            snprintf(config_path, sizeof(config_path), "Stage%i >> uvTransform >> dir", i + 1);
            read_float_array(f, config_path, material->transforms[i].transform[2], 4);

            snprintf(config_path, sizeof(config_path), "Stage%i >> uvTransform >> pos", i + 1);
            read_float_array(f, config_path, material->transforms[i].transform[3], 4);
        }
    }

    material->dummy_texture.path[0] = 0;
    material->dummy_texture.texture_filter = 3;
    material->dummy_texture.transform_index = 0;
    material->dummy_texture.type11_bool = 0;
    read_string(f, "StageTI >> texture", material->dummy_texture.path, sizeof(material->dummy_texture.path));

    // Clean up
    fclose(f);
    if (remove_file(rapified_path)) {
        errorf("Failed to remove temporary material.\n");
        return 4;
    }

    return 0;
}
