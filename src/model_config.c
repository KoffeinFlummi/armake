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


int read_animations(FILE *f, char *config_path, struct animation *animations) {
    /*
     * Reads the animation subclasses of the given config path into the struct
     * array.
     *
     * Returns 0 on success, and a positive integer on failure.
     */

    int i;
    int j;
    int success;
    char parent[2048];
    char value_path[2048];
    char value[2048];

    // Run the function for the parent class first
    success = find_parent(f, config_path, parent, sizeof(parent));
    if (success > 0 || success == -2) {
        return success;
    } else if (success == 0) {
        success = read_animations(f, parent, animations);
        if (success > 0)
            return success;
    }

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
            if (strlen(animations[j].name) == 0)
                break;
            if (strcmp(animations[j].name, anim_names[i]) == 0)
                break;
        }
        if (j == MAXANIMS)
            return 1;

        strcpy(animations[j].name, anim_names[i]);

        // Read anim type
        sprintf(value_path, "%s >> %s >> type", config_path, anim_names[i]);
        if (read_string(f, value_path, value, sizeof(value))) {
            warningf("Animation type for %s could not be found.\n", anim_names[i]);
            continue;
        }

        lower_case(value);

        if (strcmp(value, "rotation") == 0) {
            animations[j].type = TYPE_ROTATION;
        } else if (strcmp(value, "rotationx") == 0) {
            animations[j].type = TYPE_ROTATION_X;
        } else if (strcmp(value, "rotationy") == 0) {
            animations[j].type = TYPE_ROTATION_Y;
        } else if (strcmp(value, "rotationz") == 0) {
            animations[j].type = TYPE_ROTATION_Z;
        } else if (strcmp(value, "translation") == 0) {
            animations[j].type = TYPE_TRANSLATION;
        } else if (strcmp(value, "translationx") == 0) {
            animations[j].type = TYPE_TRANSLATION_X;
        } else if (strcmp(value, "translationy") == 0) {
            animations[j].type = TYPE_TRANSLATION_Y;
        } else if (strcmp(value, "translationz") == 0) {
            animations[j].type = TYPE_TRANSLATION_Z;
        } else if (strcmp(value, "direct") == 0) {
            animations[j].type = TYPE_DIRECT;
            warningf("Direct animations aren't supported yet.\n");
            continue;
        } else if (strcmp(value, "hide") == 0) {
            animations[j].type = TYPE_HIDE;
        } else {
            warningf("Unknown animation type: %s\n", value);
            continue;
        }

        // Read optional values
        animations[j].source[0] = 0;
        animations[j].selection[0] = 0;
        animations[j].axis[0] = 0;
        animations[j].begin[0] = 0;
        animations[j].end[0] = 0;
        animations[j].min_value = 0.0f;
        animations[j].max_value = 1.0f;
        animations[j].min_phase = 0.0f;
        animations[j].max_phase = 1.0f;
        animations[j].junk = 953267991;
        animations[j].always_0 = 0;
        animations[j].source_address = 0;
        animations[j].angle0 = 0.0f;
        animations[j].angle1 = 0.0f;
        animations[j].offset0 = 0.0f;
        animations[j].offset1 = 1.0f;
        animations[j].hide_value = 0.0f;
        animations[j].unhide_value = -1.0f;

#define ERROR_READING(key) warningf("Error reading %s for %s.\n", key, anim_names[i]);

        sprintf(value_path, "%s >> %s >> source", config_path, anim_names[i]);
        if (read_string(f, value_path, animations[i].source, sizeof(animations[i].source)) > 0)
            ERROR_READING("source")

        sprintf(value_path, "%s >> %s >> selection", config_path, anim_names[i]);
        if (read_string(f, value_path, animations[i].selection, sizeof(animations[i].selection)) > 0)
            ERROR_READING("selection")

        sprintf(value_path, "%s >> %s >> axis", config_path, anim_names[i]);
        if (read_string(f, value_path, animations[i].axis, sizeof(animations[i].axis)) > 0)
            ERROR_READING("axis")

        sprintf(value_path, "%s >> %s >> begin", config_path, anim_names[i]);
        if (read_string(f, value_path, animations[i].begin, sizeof(animations[i].begin)) > 0)
            ERROR_READING("begin")

        sprintf(value_path, "%s >> %s >> end", config_path, anim_names[i]);
        if (read_string(f, value_path, animations[i].end, sizeof(animations[i].end)) > 0)
            ERROR_READING("end")

        sprintf(value_path, "%s >> %s >> minValue", config_path, anim_names[i]);
        if (read_float(f, value_path, &animations[j].min_value) > 0)
            ERROR_READING("minValue")

        sprintf(value_path, "%s >> %s >> maxValue", config_path, anim_names[i]);
        if (read_float(f, value_path, &animations[j].max_value) > 0)
            ERROR_READING("maxValue")

        sprintf(value_path, "%s >> %s >> minPhase", config_path, anim_names[i]);
        if (read_float(f, value_path, &animations[j].min_phase) > 0)
            ERROR_READING("minPhase")

        sprintf(value_path, "%s >> %s >> maxPhase", config_path, anim_names[i]);
        if (read_float(f, value_path, &animations[j].max_phase) > 0)
            ERROR_READING("maxPhase")

        sprintf(value_path, "%s >> %s >> angle0", config_path, anim_names[i]);
        if (read_float(f, value_path, &animations[j].angle0) > 0)
            ERROR_READING("angle0")

        sprintf(value_path, "%s >> %s >> angle1", config_path, anim_names[i]);
        if (read_float(f, value_path, &animations[j].angle1) > 0)
            ERROR_READING("angle1")

        sprintf(value_path, "%s >> %s >> offset0", config_path, anim_names[i]);
        if (read_float(f, value_path, &animations[j].offset0) > 0)
            ERROR_READING("offset0")

        sprintf(value_path, "%s >> %s >> offset1", config_path, anim_names[i]);
        if (read_float(f, value_path, &animations[j].offset1) > 0)
            ERROR_READING("offset1")

        sprintf(value_path, "%s >> %s >> hideValue", config_path, anim_names[i]);
        if (read_float(f, value_path, &animations[j].hide_value) > 0)
            ERROR_READING("hideValue")

        sprintf(value_path, "%s >> %s >> unHideValue", config_path, anim_names[i]);
        if (read_float(f, value_path, &animations[j].unhide_value) > 0)
            ERROR_READING("unHideValue")

        sprintf(value_path, "%s >> %s >> sourceAddress", config_path, anim_names[i]);
        success = read_string(f, value_path, value, sizeof(value));
        if (success > 0) {
            ERROR_READING("sourceAddress")
        } else if (success == 0) {
            lower_case(value);

            if (strcmp(value, "clamp") == 0) {
                animations[j].source_address = SOURCE_CLAMP;
            } else if (strcmp(value, "mirror") == 0) {
                animations[j].source_address = SOURCE_MIRROR;
            } else if (strcmp(value, "loop") == 0) {
                animations[j].source_address = SOURCE_LOOP;
            } else {
                warningf("Unknown source address \"%s\" in \"%s\".\n", value, animations[j].name);
                continue;
            }
        }
    }

    return 0;
}


int read_model_config(char *path, struct skeleton *skeleton) {
    /*
     * Reads the model config information for the given model path. If no
     * model config is found, -1 is returned. 0 is returned on success
     * and a positive integer on failure.
     */

    extern int current_operation;
    extern char current_target[2048];
    FILE *f;
    int i;
    int success;
    char model_config_path[2048];
    char rapified_path[2048];
    char config_path[2048];
    char model_name[512];
    char bones[MAXBONES * 2][512] = {};
    char buffer[512];

    current_operation = OP_MODELCONFIG;
    strcpy(current_target, path);

    // Extract model.cfg path
    strncpy(model_config_path, path, sizeof(model_config_path));
    if (strrchr(model_config_path, PATHSEP) != NULL) {
        strcpy(strrchr(model_config_path, PATHSEP), "?model.cfg");
        *strrchr(model_config_path, '?') = PATHSEP;
    } else {
        strcpy(model_config_path, "model.cfg");
    }

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

    current_operation = OP_MODELCONFIG;
    strcpy(current_target, path);

    // Extract model name and convert to lower case
    if (strrchr(path, PATHSEP) != NULL)
        strcpy(model_name, strrchr(path, PATHSEP) + 1);
    else
        strcpy(model_name, path);
    *strrchr(model_name, '.') = 0;

    lower_case(model_name);

    // Open rapified file
    f = fopen(rapified_path, "r");
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
        nwarningf("model-without-prefix", "Model has a model config entry but doesn't seem to have a prefix (missing _).\n");

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
        } else {
            strcpy(buffer, "");
        }

        i = 0;
        if (strlen(buffer) > 0) {
            sprintf(config_path, "CfgSkeletons >> %s >> skeletonBones", buffer);
            success = read_array(f, config_path, (char *)bones, MAXBONES * 2, 512);
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
        success = read_array(f, config_path, (char *)bones + i * 512, MAXBONES * 2 - i, 512);
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
    }

    // Read sections
    sprintf(config_path, "CfgModels >> %s >> sectionsInherit", model_name);
    success = read_string(f, config_path, buffer, sizeof(buffer));
    if (success > 0) {
        errorf("Failed to read sections.\n");
        return success;
    } else {
        strcpy(buffer, "");
    }

    i = 0;
    if (strlen(buffer) > 0) {
        sprintf(config_path, "CfgModels >> %s >> sections", buffer);
        success = read_array(f, config_path, (char *)skeleton->sections, MAXSECTIONS, 512);
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
    success = read_array(f, config_path, (char *)skeleton->sections + i * 512, MAXSECTIONS - i, 512);
    if (success > 0) {
        errorf("Failed to read sections.\n");
        return success;
    }

    for (i = 0; i < MAXSECTIONS && skeleton->sections[i][0] != 0; i++)
        skeleton->num_sections++;

    // Read animations
    sprintf(config_path, "CfgModels >> %s >> Animations", model_name);
    fseek(f, 16, SEEK_SET);
    success = seek_config_path(f, config_path);
    if (success == 0) {
        success = read_animations(f, config_path, skeleton->animations);
        if (success > 0) {
            errorf("Failed to read animations.\n");
            return success;
        }

        for (i = 0; i < MAXANIMS && skeleton->animations[i].name[0] != 0; i++)
            skeleton->num_animations++;
    } else if (success > 0) {
        errorf("Failed to read animations.\n", success);
        return success;
    }

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
