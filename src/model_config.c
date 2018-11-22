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

#include "filesystem.h"
#include "rapify.h"
#include "utils.h"
#include "derapify.h"
#include "model_config.h"


int read_animations(FILE *f, char *config_path, struct skeleton *skeleton) {
    /*
     * Reads the animation subclasses of the given config path into the struct
     * array.
     *
     * Returns 0 on success, -1 if the path doesn't exist and a positive
     * integer on failure.
     */

    int i;
    int j;
    int k;
    int success;
    char parent[2048];
    char containing[2048];
    char value_path[2048];
    char value[2048];

    // Run the function for the parent class first
    fseek(f, 16, SEEK_SET);
    success = seek_config_path(f, config_path);
    if (success > 0) {
        return success;
    } else if (success == 0) {
        success = find_parent(f, config_path, parent, sizeof(parent));
        if (success > 0) {
            return 2;
        } else if (success == 0) {
            success = read_animations(f, parent, skeleton);
            if (success > 0)
                return success;
        }
    }

    // Check parent CfgModels entry
    strcpy(containing, config_path);
    *(strrchr(containing, '>') - 2) = 0;
    success = find_parent(f, containing, parent, sizeof(parent));
    if (success > 0) {
        return 2;
    } else if (success == 0) {
        strcat(parent, " >> Animations");
        success = read_animations(f, parent, skeleton);
        if (success > 0)
            return success;
    }

    fseek(f, 16, SEEK_SET);
    success = seek_config_path(f, config_path);
    if (success < 0)
        return -1;

    // Now go through all the animations
    char anim_names[MAXANIMS][512];
    for (i = 0; i < MAXANIMS; i++)
        anim_names[i][0] = 0;

    success = read_classes(f, config_path, (char *)anim_names, MAXANIMS, 512);
    if (success)
        return success;

    for (i = 0; i < MAXANIMS; i++) {
        if (strlen(anim_names[i]) == 0)
            break;

        for (j = 0; j < MAXANIMS; j++) {
            if (strlen(skeleton->animations[j].name) == 0)
                break;
            if (strcmp(skeleton->animations[j].name, anim_names[i]) == 0)
                break;
        }
        if (j == MAXANIMS)
            return 1;

        if (j == skeleton->num_animations) {
            skeleton->num_animations++;
        } else {
            for (k = j; k < skeleton->num_animations - 1; k++) {
                memcpy(&skeleton->animations[k], &skeleton->animations[k + 1], sizeof(struct animation));
            }
            j = skeleton->num_animations - 1;
        }

        strcpy(skeleton->animations[j].name, anim_names[i]);

        // Read anim type
        sprintf(value_path, "%s >> %s >> type", config_path, anim_names[i]);
        if (read_string(f, value_path, value, sizeof(value))) {
            lwarningf(current_target, -1, "Animation type for %s could not be found.\n", anim_names[i]);
            continue;
        }

        lower_case(value);

        if (strcmp(value, "rotation") == 0) {
            skeleton->animations[j].type = TYPE_ROTATION;
        } else if (strcmp(value, "rotationx") == 0) {
            skeleton->animations[j].type = TYPE_ROTATION_X;
        } else if (strcmp(value, "rotationy") == 0) {
            skeleton->animations[j].type = TYPE_ROTATION_Y;
        } else if (strcmp(value, "rotationz") == 0) {
            skeleton->animations[j].type = TYPE_ROTATION_Z;
        } else if (strcmp(value, "translation") == 0) {
            skeleton->animations[j].type = TYPE_TRANSLATION;
        } else if (strcmp(value, "translationx") == 0) {
            skeleton->animations[j].type = TYPE_TRANSLATION_X;
        } else if (strcmp(value, "translationy") == 0) {
            skeleton->animations[j].type = TYPE_TRANSLATION_Y;
        } else if (strcmp(value, "translationz") == 0) {
            skeleton->animations[j].type = TYPE_TRANSLATION_Z;
        } else if (strcmp(value, "direct") == 0) {
            skeleton->animations[j].type = TYPE_DIRECT;
            lwarningf(current_target, -1, "Direct animations aren't supported yet.\n");
            continue;
        } else if (strcmp(value, "hide") == 0) {
            skeleton->animations[j].type = TYPE_HIDE;
        } else {
            lwarningf(current_target, -1, "Unknown animation type: %s\n", value);
            continue;
        }

        // Read optional values
        skeleton->animations[j].source[0] = 0;
        skeleton->animations[j].selection[0] = 0;
        skeleton->animations[j].axis[0] = 0;
        skeleton->animations[j].begin[0] = 0;
        skeleton->animations[j].end[0] = 0;
        skeleton->animations[j].min_value = 0.0f;
        skeleton->animations[j].max_value = 1.0f;
        skeleton->animations[j].min_phase = 0.0f;
        skeleton->animations[j].max_phase = 1.0f;
        skeleton->animations[j].junk = 953267991;
        skeleton->animations[j].always_0 = 0;
        skeleton->animations[j].source_address = 0;
        skeleton->animations[j].angle0 = 0.0f;
        skeleton->animations[j].angle1 = 0.0f;
        skeleton->animations[j].offset0 = 0.0f;
        skeleton->animations[j].offset1 = 1.0f;
        skeleton->animations[j].hide_value = 0.0f;
        skeleton->animations[j].unhide_value = -1.0f;

#define ERROR_READING(key) lwarningf(current_target, -1, "Error reading %s for %s.\n", key, anim_names[i]);

        sprintf(value_path, "%s >> %s >> source", config_path, anim_names[i]);
        if (read_string(f, value_path, skeleton->animations[j].source, sizeof(skeleton->animations[j].source)) > 0)
            ERROR_READING("source")

        sprintf(value_path, "%s >> %s >> selection", config_path, anim_names[i]);
        if (read_string(f, value_path, skeleton->animations[j].selection, sizeof(skeleton->animations[j].selection)) > 0)
            ERROR_READING("selection")

        sprintf(value_path, "%s >> %s >> axis", config_path, anim_names[i]);
        if (read_string(f, value_path, skeleton->animations[j].axis, sizeof(skeleton->animations[j].axis)) > 0)
            ERROR_READING("axis")

        sprintf(value_path, "%s >> %s >> begin", config_path, anim_names[i]);
        if (read_string(f, value_path, skeleton->animations[j].begin, sizeof(skeleton->animations[j].begin)) > 0)
            ERROR_READING("begin")

        sprintf(value_path, "%s >> %s >> end", config_path, anim_names[i]);
        if (read_string(f, value_path, skeleton->animations[j].end, sizeof(skeleton->animations[j].end)) > 0)
            ERROR_READING("end")

        sprintf(value_path, "%s >> %s >> minValue", config_path, anim_names[i]);
        if (read_float(f, value_path, &skeleton->animations[j].min_value) > 0)
            ERROR_READING("minValue")

        sprintf(value_path, "%s >> %s >> maxValue", config_path, anim_names[i]);
        if (read_float(f, value_path, &skeleton->animations[j].max_value) > 0)
            ERROR_READING("maxValue")

        sprintf(value_path, "%s >> %s >> minPhase", config_path, anim_names[i]);
        if (read_float(f, value_path, &skeleton->animations[j].min_phase) > 0)
            ERROR_READING("minPhase")

        sprintf(value_path, "%s >> %s >> maxPhase", config_path, anim_names[i]);
        if (read_float(f, value_path, &skeleton->animations[j].max_phase) > 0)
            ERROR_READING("maxPhase")

        sprintf(value_path, "%s >> %s >> angle0", config_path, anim_names[i]);
        if (read_float(f, value_path, &skeleton->animations[j].angle0) > 0)
            ERROR_READING("angle0")

        sprintf(value_path, "%s >> %s >> angle1", config_path, anim_names[i]);
        if (read_float(f, value_path, &skeleton->animations[j].angle1) > 0)
            ERROR_READING("angle1")

        sprintf(value_path, "%s >> %s >> offset0", config_path, anim_names[i]);
        if (read_float(f, value_path, &skeleton->animations[j].offset0) > 0)
            ERROR_READING("offset0")

        sprintf(value_path, "%s >> %s >> offset1", config_path, anim_names[i]);
        if (read_float(f, value_path, &skeleton->animations[j].offset1) > 0)
            ERROR_READING("offset1")

        sprintf(value_path, "%s >> %s >> hideValue", config_path, anim_names[i]);
        if (read_float(f, value_path, &skeleton->animations[j].hide_value) > 0)
            ERROR_READING("hideValue")

        sprintf(value_path, "%s >> %s >> unHideValue", config_path, anim_names[i]);
        if (read_float(f, value_path, &skeleton->animations[j].unhide_value) > 0)
            ERROR_READING("unHideValue")

        sprintf(value_path, "%s >> %s >> sourceAddress", config_path, anim_names[i]);
        success = read_string(f, value_path, value, sizeof(value));
        if (success > 0) {
            ERROR_READING("sourceAddress")
        } else if (success == 0) {
            lower_case(value);

            if (strcmp(value, "clamp") == 0) {
                skeleton->animations[j].source_address = SOURCE_CLAMP;
            } else if (strcmp(value, "mirror") == 0) {
                skeleton->animations[j].source_address = SOURCE_MIRROR;
            } else if (strcmp(value, "loop") == 0) {
                skeleton->animations[j].source_address = SOURCE_LOOP;
            } else {
                lwarningf(current_target, -1, "Unknown source address \"%s\" in \"%s\".\n", value, skeleton->animations[j].name);
                continue;
            }
        }
    }

    return 0;
}


int sort_bones(struct bone *src, struct bone *tgt, int tgt_index, char *parent) {
    int i;
    int j;

    for (i = 0; i < MAXBONES; i++) {
        if (strlen(src[i].name) == 0)
            continue;
        if (strcmp(src[i].parent, parent) != 0)
            continue;

        memcpy(&tgt[tgt_index++], &src[i], sizeof(struct bone));
        tgt_index = sort_bones(src, tgt, tgt_index, src[i].name);
    }

    if (strlen(parent) > 0)
        return tgt_index;

    // copy the remaining bones
    for (i = 0; i < MAXBONES; i++) {
        if (strlen(src[i].name) == 0)
            break;
        for (j = 0; j < MAXBONES; j++) {
            if (strcmp(src[i].parent, tgt[j].name) == 0)
                break;
        }
        if (j == MAXBONES) {
            memcpy(&tgt[tgt_index], &src[i], sizeof(struct bone));
            tgt_index++;
        }
    }

    return tgt_index;
}


int read_model_config(char *path, struct skeleton *skeleton) {
    /*
     * Reads the model config information for the given model path. If no
     * model config is found, -1 is returned. 0 is returned on success
     * and a positive integer on failure.
     */

    extern char *current_target;
    FILE *f;
    int i;
    int success;
    char model_config_path[2048];
    char rapified_path[2048];
    char config_path[2048];
    char model_name[512];
    char bones[MAXBONES * 2][512] = {};
    char buffer[512];
    struct bone *bones_tmp;

    current_target = path;

    // Extract model.cfg path
    strncpy(model_config_path, path, sizeof(model_config_path));
    if (strrchr(model_config_path, PATHSEP) != NULL)
        strcpy(strrchr(model_config_path, PATHSEP) + 1, "model.cfg");
    else
        strcpy(model_config_path, "model.cfg");

    strcpy(rapified_path, model_config_path);
    strcat(rapified_path, ".armake.bin"); // it is assumed that this doesn't exist

    if (access(model_config_path, F_OK) == -1)
        return -1;

    // Rapify file
    success = rapify_file(model_config_path, rapified_path);
    if (success) {
        errorf("Failed to rapify model config.\n");
        return 1;
    }

    current_target = path;

    // Extract model name and convert to lower case
    if (strrchr(path, PATHSEP) != NULL)
        strcpy(model_name, strrchr(path, PATHSEP) + 1);
    else
        strcpy(model_name, path);
    *strrchr(model_name, '.') = 0;

    lower_case(model_name);

    // Open rapified file
    f = fopen(rapified_path, "rb");
    if (!f) {
        errorf("Failed to open model config.\n");
        return 2;
    }

    // Check if model entry even exists
    sprintf(config_path, "CfgModels >> %s", model_name);
    fseek(f, 16, SEEK_SET);
    success = seek_config_path(f, config_path);
    if (success > 0) {
        errorf("Failed to find model config entry.\n");
        return success;
    } else if (success < 0) {
        goto clean_up;
    }

    if (strchr(model_name, '_') == NULL)
        lnwarningf(path, -1, "model-without-prefix", "Model has a model config entry but doesn't seem to have a prefix (missing _).\n");

    // Read name
    sprintf(config_path, "CfgModels >> %s >> skeletonName", model_name);
    success = read_string(f, config_path, skeleton->name, sizeof(skeleton->name));
    if (success > 0) {
        errorf("Failed to read skeleton name.\n");
        return success;
    }

    // Read bones
    if (strlen(skeleton->name) > 0) {
        sprintf(config_path, "CfgSkeletons >> %s >> skeletonInherit", skeleton->name);
        success = read_string(f, config_path, buffer, sizeof(buffer));
        if (success > 0) {
            errorf("Failed to read bones.\n");
            return success;
        }

        int32_t temp;
        sprintf(config_path, "CfgSkeletons >> %s >> isDiscrete", skeleton->name);
        success = read_int(f, config_path, &temp);
        if (success == 0)
            skeleton->is_discrete = (temp > 0);
        else
            skeleton->is_discrete = false;

        i = 0;
        if (strlen(buffer) > 0) { // @todo: more than 1 parent
            sprintf(config_path, "CfgSkeletons >> %s >> skeletonBones", buffer);
            success = read_string_array(f, config_path, (char *)bones, MAXBONES * 2, 512);
            if (success > 0) {
                errorf("Failed to read bones.\n");
                return success;
            } else if (success == 0) {
                for (i = 0; i < MAXBONES * 2; i += 2) {
                    if (bones[i][0] == 0)
                        break;
                }
            }
        }

        sprintf(config_path, "CfgSkeletons >> %s >> skeletonBones", skeleton->name);
        success = read_string_array(f, config_path, (char *)bones + i * 512, MAXBONES * 2 - i, 512);
        if (success > 0) {
            errorf("Failed to read bones.\n");
            return success;
        }

        for (i = 0; i < MAXBONES * 2; i += 2) {
            if (bones[i][0] == 0)
                break;
            strcpy(skeleton->bones[i / 2].name, bones[i]);
            strcpy(skeleton->bones[i / 2].parent, bones[i + 1]);
            skeleton->num_bones++;
        }

        // Sort bones by parent
        bones_tmp = (struct bone *)safe_malloc(sizeof(struct bone) * MAXBONES);
        memset(bones_tmp, 0, sizeof(struct bone) * MAXBONES);

        sort_bones(skeleton->bones, bones_tmp, 0, "");
        memcpy(skeleton->bones, bones_tmp, sizeof(struct bone) * MAXBONES);

        free(bones_tmp);

        // Convert to lower case
        for (i = 0; i < MAXBONES; i++) {
            if (strlen(skeleton->bones[i].name) == 0)
                break;
            lower_case(skeleton->bones[i].name);
            lower_case(skeleton->bones[i].parent);
        }
    }

    // Read sections
    sprintf(config_path, "CfgModels >> %s >> sectionsInherit", model_name);
    success = read_string(f, config_path, buffer, sizeof(buffer));
    if (success > 0) {
        errorf("Failed to read sections.\n");
        return success;
    }

    i = 0;
    if (strlen(buffer) > 0) {
        sprintf(config_path, "CfgModels >> %s >> sections", buffer);
        success = read_string_array(f, config_path, (char *)skeleton->sections, MAXSECTIONS, 512);
        if (success > 0) {
            errorf("Failed to read sections.\n");
            return success;
        } else if (success == 0) {
            for (i = 0; i < MAXSECTIONS; i++) {
                if (skeleton->sections[i][0] == 0)
                    break;
            }
        }
    }

    sprintf(config_path, "CfgModels >> %s >> sections", model_name);
    success = read_string_array(f, config_path, (char *)skeleton->sections + i * 512, MAXSECTIONS - i, 512);
    if (success > 0) {
        errorf("Failed to read sections.\n");
        return success;
    }

    for (i = 0; i < MAXSECTIONS && skeleton->sections[i][0] != 0; i++)
        skeleton->num_sections++;

    // Read animations
    skeleton->num_animations = 0;
    sprintf(config_path, "CfgModels >> %s >> Animations", model_name);
    success = read_animations(f, config_path, skeleton);
    if (success > 0) {
        errorf("Failed to read animations.\n");
        return success;
    }

    if (strlen(skeleton->name) == 0 && skeleton->num_animations > 0)
        lwarningf(path, -1, "animated-without-skeleton", "Model doesn't have a skeleton but is animated.\n");

    // Read thermal stuff
    sprintf(config_path, "CfgModels >> %s >> htMin", model_name);
    read_float(f, config_path, &skeleton->ht_min);
    sprintf(config_path, "CfgModels >> %s >> htMax", model_name);
    read_float(f, config_path, &skeleton->ht_max);
    sprintf(config_path, "CfgModels >> %s >> afMax", model_name);
    read_float(f, config_path, &skeleton->af_max);
    sprintf(config_path, "CfgModels >> %s >> mfMax", model_name);
    read_float(f, config_path, &skeleton->mf_max);
    sprintf(config_path, "CfgModels >> %s >> mfAct", model_name);
    read_float(f, config_path, &skeleton->mf_act);
    sprintf(config_path, "CfgModels >> %s >> tBody", model_name);
    read_float(f, config_path, &skeleton->t_body);

clean_up:
    // Clean up
    fclose(f);
    if (remove_file(rapified_path)) {
        errorf("Failed to remove temporary model config.\n");
        return 3;
    }

    return 0;
}
