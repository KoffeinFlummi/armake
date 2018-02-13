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
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#endif

#include "filesystem.h"
#include "utils.h"
#include "preprocess.h"
#include "rapify.h"
#include "rapify.tab.h"


struct definitions *new_definitions() {
    struct definitions *result;

    result = (struct definitions *)safe_malloc(sizeof(struct definitions));
    result->head = NULL;

    return result;
}


struct definitions *add_definition(struct definitions *head, int type, void *content) {
    struct definition *definition;
    struct definition *tmp;

    definition = (struct definition *)safe_malloc(sizeof(struct definition));
    definition->type = type;
    definition->content = content;
    definition->next = NULL;

    if (head->head == NULL) {
        head->head = definition;
        return head;
    }

    tmp = head->head;
    while (tmp != NULL) {
        if (tmp->next == NULL) {
            tmp->next = definition;
            return head;
        }
        tmp = tmp->next;
    }

    return head;
}


struct class *new_class(char *name, char *parent, struct definitions *content, bool is_delete) {
    struct class *result;

    result = (struct class *)safe_malloc(sizeof(struct class));
    result->name = name;
    result->parent = parent;
    result->is_delete = is_delete;
    result->content = content;

    return result;
}


struct variable *new_variable(int type, char *name, struct expression *expression) {
    struct variable *result;

    result = (struct variable *)safe_malloc(sizeof(struct variable));
    result->type = type;
    result->name = name;
    result->expression = expression;

    return result;
}


struct expression *new_expression(int type, void *value) {
    struct expression *result;

    result = (struct expression *)safe_malloc(sizeof(struct expression));
    result->type = type;
    result->string_value = NULL;
    result->head = NULL;
    result->next = NULL;

    if (type == TYPE_INT) {
        result->int_value = *((int *)value);
    } else if (type == TYPE_FLOAT) {
        result->float_value = *((float *)value);
    } else if (type == TYPE_STRING) {
        result->string_value = (char *)value;
    } else if (type == TYPE_ARRAY || type == TYPE_ARRAY_EXPANSION) {
        result->head = (struct expression *)value;
    }

    return result;
}


struct expression *add_expression(struct expression *head, struct expression *new) {
    struct expression *tmp;

    tmp = head;
    while (tmp != NULL) {
        if (tmp->next == NULL) {
            tmp->next = new;
            break;
        }

        tmp = tmp->next;
    }

    return head;
}


void free_expression(struct expression *expr) {
    if (expr == NULL) { return; }
    free(expr->string_value);
    free_expression(expr->head);
    free_expression(expr->next);
    free(expr);
}


void free_variable(struct variable *var) {
    free(var->name);
    free_expression(var->expression);
    free(var);
}


void free_definition(struct definition *definition) {
    if (definition == NULL) { return; }

    free_definition(definition->next);

    if (definition->type == TYPE_VAR)
        free_variable((struct variable *)definition->content);
    else
        free_class((struct class *)definition->content);

    free(definition);
}


void free_class(struct class *class) {
    free(class->name);
    free(class->parent);

    if (class->content != NULL && class->content != (struct definitions *)-1) {
        free_definition(class->content->head);
        free(class->content);
    }

    free(class);
}


void rapify_expression(struct expression *expr, FILE *f_target) {
    struct expression *tmp;
    uint32_t num_entries;

    if (expr->type == TYPE_ARRAY) {
        num_entries = 0;
        tmp = expr->head;
        while (tmp != NULL) {
            num_entries++;
            tmp = tmp->next;
        }

        write_compressed_int(num_entries, f_target);

        tmp = expr->head;
        while (tmp != NULL) {
            fputc((char)((tmp->type == TYPE_STRING) ? 0 :
                ((tmp->type == TYPE_FLOAT) ? 1 :
                ((tmp->type == TYPE_INT) ? 2 : 3))), f_target);
            rapify_expression(tmp, f_target);
            tmp = tmp->next;
        }
    } else if (expr->type == TYPE_INT) {
        fwrite(&expr->int_value, 4, 1, f_target);
    } else if (expr->type == TYPE_FLOAT) {
        fwrite(&expr->float_value, 4, 1, f_target);
    } else {
        fwrite(expr->string_value, strlen(expr->string_value) + 1, 1, f_target);
    }
}


void rapify_variable(struct variable *var, FILE *f_target) {
    struct expression *tmp;
    uint32_t num_entries;

    if (var->type == TYPE_VAR) {
        fputc(1, f_target);
        fputc((char)((var->expression->type == TYPE_STRING) ? 0 : ((var->expression->type == TYPE_FLOAT) ? 1 : 2 )), f_target);
    } else {
        fputc((char)((var->type == TYPE_ARRAY) ? 2 : 5), f_target);
        if (var->type == TYPE_ARRAY_EXPANSION) {
            num_entries = 0;
            tmp = var->expression->head;
            while (tmp != NULL) {
                num_entries++;
                tmp = tmp->next;
            }

            fwrite(&num_entries, 4, 1, f_target);
        }
    }

    fwrite(var->name, strlen(var->name) + 1, 1, f_target);
    rapify_expression(var->expression, f_target);
}


void rapify_class(struct class *class, FILE *f_target) {
    struct definition *tmp;
    uint32_t fp_temp;
    uint32_t num_entries = 0;

    if (class->content == NULL) {
        // extern or delete class
        fputc((char)(class->is_delete ? 4 : 3), f_target);
        fwrite(class->name, strlen(class->name) + 1, 1, f_target);
        return;
    }

    if (class->parent)
        fwrite(class->parent, strlen(class->parent) + 1, 1, f_target);
    else
        fputc(0, f_target);

    tmp = class->content->head;
    while (tmp != NULL) {
        num_entries++;
        tmp = tmp->next;
    }

    write_compressed_int(num_entries, f_target);

    tmp = class->content->head;
    while (tmp != NULL) {
        if (tmp->type == TYPE_VAR) {
            rapify_variable((struct variable *)tmp->content, f_target);
        } else {
            if (((struct class *)(tmp->content))->content != NULL) {
                fputc(0, f_target);
                fwrite(((struct class *)(tmp->content))->name,
                    strlen(((struct class *)(tmp->content))->name) + 1, 1, f_target);
                ((struct class *)(tmp->content))->offset_location = ftell(f_target);
                fwrite("\0\0\0\0", 4, 1, f_target);
            } else {
                rapify_class(tmp->content, f_target);
            }
        }

        tmp = tmp->next;
    }

    tmp = class->content->head;
    while (tmp != NULL) {
        if (tmp->type == TYPE_CLASS && ((struct class *)(tmp->content))->content != NULL) {
            fp_temp = ftell(f_target);
            fseek(f_target, ((struct class *)(tmp->content))->offset_location, SEEK_SET);
            fwrite(&fp_temp, sizeof(uint32_t), 1, f_target);
            fseek(f_target, 0, SEEK_END);

            rapify_class(tmp->content, f_target);
        }

        tmp = tmp->next;
    }
}

int rapify_file(char *source, char *target) {
    /*
     * Resolves macros/includes and rapifies the given file. If source and
     * target are identical, the target is overwritten.
     *
     * Returns 0 on success and a positive integer on failure.
     */

    extern char *current_target;
    FILE *f_temp;
    FILE *f_target;
    int i;
    int datasize;
    int success;
    char buffer[4096];
    uint32_t enum_offset = 0;
    struct constants *constants;
    struct lineref *lineref;

    current_target = source;

    // Check if the file is already rapified
    f_temp = fopen(source, "rb");
    if (!f_temp) {
        errorf("Failed to open %s.\n", source);
        return 1;
    }

    fread(buffer, 4, 1, f_temp);
    if (strncmp(buffer, "\0raP", 4) == 0) {
        if ((strcmp(source, target)) == 0) {
            fclose(f_temp);
            return 0;
        }

        if (strcmp(target, "-") == 0) {
            f_target = stdout;
        } else {
            f_target = fopen(target, "wb");
            if (!f_target) {
                errorf("Failed to open %s.\n", target);
                fclose(f_temp);
                return 2;
            }
        }

        fseek(f_temp, 0, SEEK_END);
        datasize = ftell(f_temp);

        fseek(f_temp, 0, SEEK_SET);
        for (i = 0; datasize - i >= sizeof(buffer); i += sizeof(buffer)) {
            fread(buffer, sizeof(buffer), 1, f_temp);
            fwrite(buffer, sizeof(buffer), 1, f_target);
        }
        fread(buffer, datasize - i, 1, f_temp);
        fwrite(buffer, datasize - i, 1, f_target);

        fclose(f_temp);
        if (strcmp(target, "-") != 0)
            fclose(f_target);

        return 0;
    } else {
        fclose(f_temp);
    }

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

    for (i = 0; i < MAXINCLUDES; i++)
        include_stack[i][0] = 0;

    constants = constants_init();

    lineref = (struct lineref *)safe_malloc(sizeof(struct lineref));
    lineref->num_files = 0;
    lineref->num_lines = 0;
    lineref->file_index = (uint32_t *)safe_malloc(sizeof(uint32_t) * LINEINTERVAL);
    lineref->line_number = (uint32_t *)safe_malloc(sizeof(uint32_t) * LINEINTERVAL);

    success = preprocess(source, f_temp, constants, lineref);

    current_target = source;

    if (success) {
        errorf("Failed to preprocess %s.\n", source);
        fclose(f_temp);
#ifdef _WIN32
        DeleteFile(temp_name);
#endif
        return success;
    }

#if 0
    FILE *f_dump;

    char dump_name[2048];
    sprintf(dump_name, "armake_preprocessed_%u.dump", (unsigned)time(NULL));
    printf("Done with preprocessing, dumping preprocessed config to %s.\n", dump_name);

    f_dump = fopen(dump_name, "wb");
    fseek(f_temp, 0, SEEK_END);
    datasize = ftell(f_temp);

    fseek(f_temp, 0, SEEK_SET);
    for (i = 0; datasize - i >= sizeof(buffer); i += sizeof(buffer)) {
        fread(buffer, sizeof(buffer), 1, f_temp);
        fwrite(buffer, sizeof(buffer), 1, f_dump);
    }

    fread(buffer, datasize - i, 1, f_temp);
    fwrite(buffer, datasize - i, 1, f_dump);

    fclose(f_dump);
#endif

    fseek(f_temp, 0, SEEK_SET);
    struct class *result;
    result = parse_file(f_temp, lineref);

    if (result == NULL) {
        errorf("Failed to parse config.\n");
        return 1;
    }

#ifdef _WIN32
    char temp_name2[2048];
#endif

    // Rapify file
    if (strcmp(target, "-") == 0) {
#ifdef _WIN32
        if (!GetTempFileName(".", "amk", 0, temp_name2)) {
            errorf("Failed to get temp file name (system error %i).\n", GetLastError());
            return 1;
        }
        f_target = fopen(temp_name, "wb+");
#else
        f_target = tmpfile();
#endif

        if (!f_target) {
            errorf("Failed to open temp file.\n");
#ifdef _WIN32
            DeleteFile(temp_name2);
#endif
            return 1;
        }
    } else {
        f_target = fopen(target, "wb+");
        if (!f_target) {
            errorf("Failed to open %s.\n", target);
            fclose(f_temp);
            return 2;
        }
    }
    fwrite("\0raP", 4, 1, f_target);
    fwrite("\0\0\0\0\x08\0\0\0", 8, 1, f_target);
    fwrite(&enum_offset, 4, 1, f_target); // this is replaced later

    rapify_class(result, f_target);

    enum_offset = ftell(f_target);
    fwrite("\0\0\0\0", 4, 1, f_target); // fuck enums
    fseek(f_target, 12, SEEK_SET);
    fwrite(&enum_offset, 4, 1, f_target);

    if (strcmp(target, "-") == 0) {
        fseek(f_target, 0, SEEK_END);
        datasize = ftell(f_target);

        fseek(f_target, 0, SEEK_SET);
        for (i = 0; datasize - i >= sizeof(buffer); i += sizeof(buffer)) {
            fread(buffer, sizeof(buffer), 1, f_target);
            fwrite(buffer, sizeof(buffer), 1, stdout);
        }
        fread(buffer, datasize - i, 1, f_target);
        fwrite(buffer, datasize - i, 1, stdout);
    }

    fclose(f_temp);
    fclose(f_target);

#ifdef _WIN32
    DeleteFile(temp_name);
    DeleteFile(temp_name2);
#endif

    constants_free(constants);

    free(lineref->file_index);
    free(lineref->line_number);
    free(lineref);

    free_class(result);

    return 0;
}
