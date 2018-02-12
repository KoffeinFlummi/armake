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
#include <stdbool.h>

#include "args.h"
#include "utils.h"
#include "img2paa.h"
#include "paa2img.h"
#include "binarize.h"
#include "build.h"
#include "unpack.h"
#include "derapify.h"
#include "filesystem.h"
#include "keygen.h"
#include "sign.h"


void print_usage() {
    printf("armake\n"
           "\n"
           "Usage:\n"
           "    armake binarize [-f] [-w <wname>] [-i <includefolder>] <source> [<target>]\n"
           "    armake build [-f] [-p] [-w <wname>] [-i <includefolder>] [-x <xlist>] [-e <headerextension>] [-k <privatekey>] <folder> <pbo>\n"
           "    armake inspect <pbo>\n"
           "    armake unpack [-f] [-i <includepattern>] [-x <excludepattern>] <pbo> <folder>\n"
           "    armake cat <pbo> <name>\n"
           "    armake derapify [-f] [-d <indentation>] [<source> [<target>]]\n"
           "    armake keygen [-f] <keyname>\n"
           "    armake sign [-f] <privatekey> <pbo>\n"
           "    armake paa2img [-f] <source> <target>\n"
           "    armake img2paa [-f] [-z] [-t <paatype>] <source> <target>\n"
           "    armake (-h | --help)\n"
           "    armake (-v | --version)\n"
           "\n"
           "Commands:\n"
           "    binarize    Binarize a file.\n"
           "    build       Pack a folder into a PBO.\n"
           "    inspect     Inspect a PBO and list contained files.\n"
           "    unpack      Unpack a PBO into a folder.\n"
           "    cat         Read the named file from the target PBO to stdout.\n"
           "    derapify    Derapify a config. Pass no target for stdout and no source for stdin.\n"
           "    keygen      Generate a keypair with the specified path (extensions are added).\n"
           "    sign        Sign a PBO with the given private key.\n"
           "    paa2img     Convert PAA to image (PNG only).\n"
           "    img2paa     Convert image to PAA.\n"
           "\n"
           "Options:\n"
           "    -f --force      Overwrite the target file/folder if it already exists.\n"
           "    -p --packonly   Don't binarize models, configs etc.\n"
           "    -w --warning    Warning to disable (repeatable).\n"
           "    -i --include    Folder to search for includes, defaults to CWD (repeatable).\n"
           "                        For unpack: pattern to include in output folder (repeatable).\n"
           "    -x --exclude    Glob patterns to exclude from PBO (repeatable).\n"
           "                        For unpack: pattern to exclude from output folder (repeatable).\n"
           "    -e --headerext  Header extension (repeatable).\n"
           "                        Example: foo=bar\n"
           "    -k --key        Private key to use for signing the PBO.\n"
           "    -d --indent     String to use for indentation. "    " (4 spaces) by default.\n"
           "    -z --compress   Compress final PAA where possible.\n"
           "    -t --type       PAA type. One of: DXT1, DXT3, DXT5, ARGB4444, ARGB1555, AI88\n"
           "                        Currently only DXT1 and DXT5 are implemented.\n"
           "    -h --help       Show usage information and exit.\n"
           "    -v --version    Print the version number and exit.\n"
           "\n"
           "Warnings:\n"
           "    By default, armake prints all warnings. You can mute trivial warnings\n"
           "    using the name that is printed along with them.\n"
           "\n"
           "    Example: \"-w unquoted-string\" disables warnings about improperly quoted\n"
           "             strings.\n"
           "\n"
           "BI tools on Windows:\n"
           "    Since armake's P3D converter is not finished yet, armake attempts to find\n"
           "    and use BI's binarize.exe on Windows systems. If you don't want this to\n"
           "    happen and use armake's instead, pass the environment variable NATIVEBIN.\n"
           "\n"
           "    Since binarize.exe's output is usually excessively verbose, it is hidden\n"
           "    by default. Pass BIOUTPUT to display it.\n");
}


void append(char ***array, int *num, char *value) {
    (*num)++;

    if (*array == NULL)
        *array = (char **)safe_malloc(sizeof(char *));
    else
        *array = (char **)safe_realloc(*array, sizeof(char *) * *num);

    (*array)[*num - 1] = value;
}


int read_args(int argc, char *argv[]) {
    extern struct arguments args;
    int i, j;

    const struct arg_option bool_options[] = {
        { "-f", "--force", &args.force, NULL },
        { "-p", "--packonly", &args.packonly, NULL },
        { "-z", "--compress", &args.compress, NULL }
    };

    const struct arg_option single_options[] = {
        { "-k", "--key", &args.privatekey, NULL },
        { "-d", "--indent", &args.indent, NULL },
        { "-t", "--type", &args.paatype, NULL }
    };

    const struct arg_option multi_options[] = {
        { "-w", "--warning", &args.mutedwarnings, &args.num_mutedwarnings },
        { "-i", "--include", &args.includefolders, &args.num_includefolders },
        { "-x", "--exclude", &args.excludefiles, &args.num_excludefiles },
        { "-e", "--headerext", &args.headerextensions, &args.num_headerextensions }
    };

    for (i = 1; i < argc; i++) {
        if (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
            print_usage();
            return -1;
        }

        if (strcmp("-v", argv[i]) == 0 || strcmp("--version", argv[i]) == 0) {
            puts(VERSION);
            return -1;
        }

        for (j = 0; j < sizeof(bool_options) / sizeof(struct arg_option); j++) {
            if (strcmp(bool_options[j].short_name, argv[i]) == 0 ||
                    strcmp(bool_options[j].long_name, argv[i]) == 0) {
                *(bool *)(bool_options[j].value) = true;
                break;
            }
        }

        if (j < sizeof(bool_options) / sizeof(struct arg_option))
            continue;

        for (j = 0; j < sizeof(single_options) / sizeof(struct arg_option); j++) {
            if (strcmp(single_options[j].short_name, argv[i]) == 0 ||
                    strcmp(single_options[j].long_name, argv[i]) == 0) {
                if (++i == argc)
                    return 1;
                *(char **)(single_options[j].value) = argv[i];
                break;
            }
        }

        if (j < sizeof(single_options) / sizeof(struct arg_option))
            continue;

        for (j = 0; j < sizeof(multi_options) / sizeof(struct arg_option); j++) {
            if (strcmp(multi_options[j].short_name, argv[i]) == 0 ||
                    strcmp(multi_options[j].long_name, argv[i]) == 0) {
                if (++i == argc)
                    return 1;
                append(multi_options[j].value, multi_options[j].length, argv[i]);
                break;
            }
        }

        if (j < sizeof(multi_options) / sizeof(struct arg_option))
            continue;

        append(&args.positionals, &args.num_positionals, argv[i]);
    }

    // printf("Positionals: %i\n", args.num_positionals);
    // for (i = 0; i < args.num_positionals; i++) {
    //     printf("    %i: %s\n", i, args.positionals[i]);
    // }

    // for (i = 0; i < sizeof(bool_options) / sizeof(struct arg_option); i++) {
    //     printf("%s %s: %i\n", bool_options[i].short_name, bool_options[i].long_name,
    //         *(bool *)(bool_options[i].value));
    // }

    // for (i = 0; i < sizeof(single_options) / sizeof(struct arg_option); i++) {
    //     printf("%s %s: %s\n", single_options[i].short_name, single_options[i].long_name,
    //         *(char **)(single_options[i].value));
    // }

    // for (i = 0; i < sizeof(multi_options) / sizeof(struct arg_option); i++) {
    //     printf("%s %s: %i\n", multi_options[i].short_name, multi_options[i].long_name,
    //         *multi_options[i].length);
    //     for (j = 0; j < *multi_options[i].length; j++) {
    //         printf("    %i: %s\n", j, (*(char ***)(multi_options[i].value))[j]);
    //     }
    // }

    return 0;
}


int main(int argc, char *argv[]) {
    extern struct arguments args;
    int i;
    int success;

    append(&args.includefolders, &args.num_includefolders, ".");

    success = read_args(argc, argv);

    if (success < 0)
        return 0;
    if (success > 0)
        goto error;

    for (i = 0; i < args.num_positionals; i++) {
        if (args.positionals[i][strlen(args.positionals[i]) - 1] == PATHSEP)
            args.positionals[i][strlen(args.positionals[i]) - 1] = 0;
    }

    for (i = 0; i < args.num_includefolders; i++) {
        if (args.includefolders[i][strlen(args.includefolders[i]) - 1] == PATHSEP)
            args.includefolders[i][strlen(args.includefolders[i]) - 1] = 0;
    }

    if (args.num_positionals == 0 || args.num_positionals > 3)
        goto error;

    if (strcmp(args.positionals[0], "binarize") == 0)
        success = cmd_binarize();
    else if (strcmp(args.positionals[0], "build") == 0)
        success = cmd_build();
    else if (strcmp(args.positionals[0], "inspect") == 0)
        success = cmd_inspect();
    else if (strcmp(args.positionals[0], "unpack") == 0)
        success = cmd_unpack();
    else if (strcmp(args.positionals[0], "cat") == 0)
        success = cmd_cat();
    else if (strcmp(args.positionals[0], "derapify") == 0)
        success = cmd_derapify();
    else if (strcmp(args.positionals[0], "keygen") == 0)
        success = cmd_keygen();
    else if (strcmp(args.positionals[0], "sign") == 0)
        success = cmd_sign();
    else if (strcmp(args.positionals[0], "paa2img") == 0)
        success = cmd_paa2img();
    else if (strcmp(args.positionals[0], "img2paa") == 0)
        success = cmd_img2paa();
    else
        goto error;

    goto done;

error:
    success = 128;
    print_usage();

done:
    if (args.positionals)
        free(args.positionals);
    if (args.mutedwarnings)
        free(args.mutedwarnings);
    if (args.includefolders)
        free(args.includefolders);
    if (args.excludefiles)
        free(args.excludefiles);
    if (args.headerextensions)
        free(args.headerextensions);

    return success;
}
