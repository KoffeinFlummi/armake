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

#include "args.h"
#include "filesystem.h"
#include "rapify.h"
#include "preprocess.h"
#include "utils.h"
#include "derapify.h"
#include "matrix.h"
#include "material.h"


const struct shader_ref pixelshaders[153] = {
    { 0, "Normal" },
    { 1, "NormalDXTA" },
    { 2, "NormalMap" },
    { 3, "NormalMapThrough" },
    { 4, "NormalMapGrass" },
    { 5, "NormalMapDiffuse" },
    { 6, "Detail" },
    { 7, "Interpolation" },
    { 8, "Water" },
    { 9, "WaterSimple" },
    { 10, "White" },
    { 11, "WhiteAlpha" },
    { 12, "AlphaShadow" },
    { 13, "AlphaNoShadow" },
    { 14, "Dummy0" },
    { 15, "DetailMacroAS" },
    { 16, "NormalMapMacroAS" },
    { 17, "NormalMapDiffuseMacroAS" },
    { 18, "NormalMapSpecularMap" },
    { 19, "NormalMapDetailSpecularMap" },
    { 20, "NormalMapMacroASSpecularMap" },
    { 21, "NormalMapDetailMacroASSpecularMap" },
    { 22, "NormalMapSpecularDIMap" },
    { 23, "NormalMapDetailSpecularDIMap" },
    { 24, "NormalMapMacroASSpecularDIMap" },
    { 25, "NormalMapDetailMacroASSpecularDIMap" },
    { 26, "Terrain1" },
    { 27, "Terrain2" },
    { 28, "Terrain3" },
    { 29, "Terrain4" },
    { 30, "Terrain5" },
    { 31, "Terrain6" },
    { 32, "Terrain7" },
    { 33, "Terrain8" },
    { 34, "Terrain9" },
    { 35, "Terrain10" },
    { 36, "Terrain11" },
    { 37, "Terrain12" },
    { 38, "Terrain13" },
    { 39, "Terrain14" },
    { 40, "Terrain15" },
    { 41, "TerrainSimple1" },
    { 42, "TerrainSimple2" },
    { 43, "TerrainSimple3" },
    { 44, "TerrainSimple4" },
    { 45, "TerrainSimple5" },
    { 46, "TerrainSimple6" },
    { 47, "TerrainSimple7" },
    { 48, "TerrainSimple8" },
    { 49, "TerrainSimple9" },
    { 50, "TerrainSimple10" },
    { 51, "TerrainSimple11" },
    { 52, "TerrainSimple12" },
    { 53, "TerrainSimple13" },
    { 54, "TerrainSimple14" },
    { 55, "TerrainSimple15" },
    { 56, "Glass" },
    { 57, "NonTL" },
    { 58, "NormalMapSpecularThrough" },
    { 59, "Grass" },
    { 60, "NormalMapThroughSimple" },
    { 61, "NormalMapSpecularThroughSimple" },
    { 62, "Road" },
    { 63, "Shore" },
    { 64, "ShoreWet" },
    { 65, "Road2Pass" },
    { 66, "ShoreFoam" },
    { 67, "NonTLFlare" },
    { 68, "NormalMapThroughLowEnd" },
    { 69, "TerrainGrass1" },
    { 70, "TerrainGrass2" },
    { 71, "TerrainGrass3" },
    { 72, "TerrainGrass4" },
    { 73, "TerrainGrass5" },
    { 74, "TerrainGrass6" },
    { 75, "TerrainGrass7" },
    { 76, "TerrainGrass8" },
    { 77, "TerrainGrass9" },
    { 78, "TerrainGrass10" },
    { 79, "TerrainGrass11" },
    { 80, "TerrainGrass12" },
    { 81, "TerrainGrass13" },
    { 82, "TerrainGrass14" },
    { 83, "TerrainGrass15" },
    { 84, "Crater1" },
    { 85, "Crater2" },
    { 86, "Crater3" },
    { 87, "Crater4" },
    { 88, "Crater5" },
    { 89, "Crater6" },
    { 90, "Crater7" },
    { 91, "Crater8" },
    { 92, "Crater9" },
    { 93, "Crater10" },
    { 94, "Crater11" },
    { 95, "Crater12" },
    { 96, "Crater13" },
    { 97, "Crater14" },
    { 98, "Sprite" },
    { 99, "SpriteSimple" },
    { 100, "Cloud" },
    { 101, "Horizon" },
    { 102, "Super" },
    { 103, "Multi" },
    { 104, "TerrainX" },
    { 105, "TerrainSimpleX" },
    { 106, "TerrainGrassX" },
    { 107, "Tree" },
    { 108, "TreePRT" },
    { 109, "TreeSimple" },
    { 110, "Skin" },
    { 111, "CalmWater" },
    { 112, "TreeAToC" },
    { 113, "GrassAToC" },
    { 114, "TreeAdv" },
    { 115, "TreeAdvSimple" },
    { 116, "TreeAdvTrunk" },
    { 117, "TreeAdvTrunkSimple" },
    { 118, "TreeAdvAToC" },
    { 119, "TreeAdvSimpleAToC" },
    { 120, "TreeSN" },
    { 121, "SpriteExtTi" },
    { 122, "TerrainSNX" },
    { 123, "InterpolationAlpha" },
    { 124, "VolCloud" },
    { 125, "VolCloudSimple" },
    { 126, "UnderwaterOcclusion" },
    { 127, "SimulWeatherClouds" },
    { 128, "SimulWeatherCloudsWithLightning" },
    { 129, "SimulWeatherCloudsCPU" },
    { 130, "SimulWeatherCloudsWithLightningCPU" },
    { 131, "SuperExt" },
    { 132, "SuperHair" },
    { 133, "SuperHairAtoC" },
    { 134, "Caustics" },
    { 135, "Refract" },
    { 136, "SpriteRefract" },
    { 137, "SpriteRefractSimple" },
    { 138, "SuperAToC" },
    { 139, "NonTLFlareNew" },
    { 140, "NonTLFlareLight" },
    { 141, "TerrainNoDetailX" },
    { 142, "TerrainNoDetailSNX" },
    { 143, "TerrainSimpleSNX" },
    { 144, "NormalPiP" },
    { 145, "NonTLFlareNewNoOcclusion" },
    { 146, "Empty" },
    { 147, "Point" },
    { 148, "TreeAdvTrans"  },
    { 149, "TreeAdvTransAToC" },
    { 150, "Collimator" },
    { 151, "LODDiag" },
    { 152, "DepthOnly" }
};

const struct shader_ref vertexshaders[45] = {
    { 0, "Basic" },
    { 1, "NormalMap" },
    { 2, "NormalMapDiffuse" },
    { 3, "Grass" },
    { 4, "Dummy2" },
    { 5, "Dummy3" },
    { 6, "ShadowVolume" },
    { 7, "Water" },
    { 8, "WaterSimple" },
    { 9, "Sprite" },
    { 10, "Point" },
    { 11, "NormalMapThrough" },
    { 12, "Dummy3" },
    { 13, "Terrain" },
    { 14, "BasicAS" },
    { 15, "NormalMapAS" },
    { 16, "NormalMapDiffuseAS" },
    { 17, "Glass" },
    { 18, "NormalMapSpecularThrough" },
    { 19, "NormalMapThroughNoFade" },
    { 20, "NormalMapSpecularThroughNoFade" },
    { 21, "Shore" },
    { 22, "TerrainGrass" },
    { 23, "Super" },
    { 24, "Multi" },
    { 25, "Tree" },
    { 26, "TreeNoFade" },
    { 27, "TreePRT" },
    { 28, "TreePRTNoFade" },
    { 29, "Skin" },
    { 30, "CalmWater" },
    { 31, "TreeAdv" },
    { 32, "TreeAdvTrunk" },
    { 33, "VolCloud" },
    { 34, "Road" },
    { 35, "UnderwaterOcclusion" },
    { 36, "SimulWeatherClouds" },
    { 37, "SimulWeatherCloudsCPU" },
    { 38, "SpriteOnSurface" },
    { 39, "TreeAdvModNormals" },
    { 40, "Refract" },
    { 41, "SimulWeatherCloudsGS" },
    { 42, "BasicFade" },
    { 43, "Star" },
    { 44, "TreeAdvNoFade" },
};


int read_material(struct material *material) {
    /*
     * Reads the material information for the given material struct.
     * Returns 0 on success and a positive integer on failure.
     */

    extern char *current_target;
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

    // Write default values
    material->type = MATERIALTYPE;
    material->depr_1 = 1;
    material->depr_2 = 1;
    material->depr_3 = 1;
    material->emissive = default_color;
    material->ambient = default_color;
    material->diffuse = default_color;
    material->forced_diffuse = default_color;
    material->specular = default_color;
    material->specular2 = default_color;
    material->specular_power = 1.0f;
    material->pixelshader_id = 0;
    material->vertexshader_id = 0;
    material->num_textures = 1;
    material->num_transforms = 1;

    material->textures = (struct stage_texture *)safe_malloc(sizeof(struct stage_texture) * material->num_textures);
    material->transforms = (struct stage_transform *)safe_malloc(sizeof(struct stage_transform) * material->num_transforms);

    material->textures[0].path[0] = 0;
    material->textures[0].texture_filter = 3;
    material->textures[0].transform_index = 0;
    material->textures[0].type11_bool = 0;

    material->transforms[0].uv_source = 1;
    memset(material->transforms[0].transform, 0, 12 * sizeof(float));
    memcpy(material->transforms[0].transform, &identity_matrix, sizeof(identity_matrix));

    material->dummy_texture.path[0] = 0;
    material->dummy_texture.texture_filter = 3;
    material->dummy_texture.transform_index = 0;
    material->dummy_texture.type11_bool = 0;

    if (find_file(temp, "", actual_path)) {
        lwarningf(current_target, -1, "Failed to find material \"%s\".\n", temp);
        return 1;
    }

    current_target = temp;

    strcpy(rapified_path, actual_path);
    strcat(rapified_path, ".armake.bin"); // it is assumed that this doesn't exist

    // Rapify file
    if (rapify_file(actual_path, rapified_path)) {
        lwarningf(current_target, -1, "Failed to rapify %s.\n", actual_path);
        return 2;
    }

    current_target = material->path;

    // Open rapified file
    f = fopen(rapified_path, "rb");
    if (!f) {
        lwarningf(current_target, -1, "Failed to open rapified material.\n");
        return 3;
    }

    // Read colors
    read_float_array(f, "emmisive", (float *)&material->emissive, 4); // "Did you mean: emissive?"
    read_float_array(f, "ambient", (float *)&material->ambient, 4);
    read_float_array(f, "diffuse", (float *)&material->diffuse, 4);
    read_float_array(f, "forcedDiffuse", (float *)&material->forced_diffuse, 4);
    read_float_array(f, "specular", (float *)&material->specular, 4);
    material->specular2 = material->specular;

    read_float(f, "specularPower", &material->specular_power);

    // Read shaders
    if (!read_string(f, "PixelShaderID", shader, sizeof(shader))) {
        for (i = 0; i < sizeof(pixelshaders) / sizeof(struct shader_ref); i++) {
            if (stricmp((char *)pixelshaders[i].name, shader) == 0)
                break;
        }
        if (i == sizeof(pixelshaders) / sizeof(struct shader_ref)) {
            lwarningf(current_target, -1, "Unrecognized pixel shader: \"%s\", assuming \"Normal\".\n", shader);
            i = 0;
        }
        material->pixelshader_id = pixelshaders[i].id;
    }

    if (!read_string(f, "VertexShaderID", shader, sizeof(shader))) {
        for (i = 0; i < sizeof(vertexshaders) / sizeof(struct shader_ref); i++) {
            if (stricmp((char *)vertexshaders[i].name, shader) == 0)
                break;
        }
        if (i == sizeof(vertexshaders) / sizeof(struct shader_ref)) {
            lwarningf(current_target, -1, "Unrecognized vertex shader: \"%s\", assuming \"Basic\".\n", shader);
            i = 0;
        }
        material->vertexshader_id = vertexshaders[i].id;
    }

    // Read stages
    for (i = 1; i < MAXSTAGES; i++) {
        snprintf(config_path, sizeof(config_path), "Stage%i >> texture", i);
        if (read_string(f, config_path, temp, sizeof(temp)))
            break;
        material->num_textures++;
        material->num_transforms++;
    }

    free(material->textures);
    free(material->transforms);
    material->textures = (struct stage_texture *)safe_malloc(sizeof(struct stage_texture) * material->num_textures);
    material->transforms = (struct stage_transform *)safe_malloc(sizeof(struct stage_transform) * material->num_transforms);

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
        memset(material->transforms[i].transform, 0, 12 * sizeof(float));
        memcpy(material->transforms[i].transform, &identity_matrix, sizeof(identity_matrix));

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

    read_string(f, "StageTI >> texture", material->dummy_texture.path, sizeof(material->dummy_texture.path));

    // Clean up
    fclose(f);
    if (remove_file(rapified_path)) {
        lwarningf(current_target, -1, "Failed to remove temporary material.\n");
        return 4;
    }

    return 0;
}
