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

#include "docopt.h"
#include "img2paa.h"
#include "paa2img.h"
#include "binarize.h"
#include "build.h"
#include "unpack.h"
#include "derapify.h"
#include "filesystem.h"
#include "keygen.h"
#include "sign.h"


int main(int argc, char *argv[]) {
    extern DocoptArgs args;
    extern char exclude_files[MAXEXCLUDEFILES][512];
    extern char include_folders[MAXINCLUDEFOLDERS][512];
    extern char muted_warnings[MAXWARNINGS][512];
    int i;
    int j;
    char *halp[] = {argv[0], "-h"};

    args = docopt(argc, argv, 1, VERSION);

    // Docopt doesn't yet support positional arguments
    if (argc < ((args.keygen || args.inspect) ? 3 : 4))
        docopt(2, halp, 1, VERSION);

    if (!args.keygen && !args.inspect) {
        args.source = argv[argc - 2];
        if (args.source[strlen(args.source) - 1] == PATHSEP)
            args.source[strlen(args.source) - 1] = 0;

        if (args.source[0] == '-' && strlen(args.source) > 1)
            docopt(2, halp, 1, VERSION);
    }

    args.target = argv[argc - 1];
    if (args.target[strlen(args.target) - 1] == PATHSEP)
        args.target[strlen(args.target) - 1] = 0;

    if (args.target[0] == '-' && strlen(args.target) > 1)
        docopt(2, halp, 1, VERSION);


    // @todo
    strcpy(include_folders[0], ".");
    for (i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--exclude") == 0) {
            for (j = 0; j < MAXEXCLUDEFILES && exclude_files[j][0] != 0; j++);
            strncpy(exclude_files[j], argv[i + 1], sizeof(exclude_files[j]));
        }
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--include") == 0) {
            for (j = 0; j < MAXINCLUDEFOLDERS && include_folders[j][0] != 0; j++);
            strncpy(include_folders[j], argv[i + 1], sizeof(include_folders[j]));
            if (include_folders[j][strlen(include_folders[j]) - 1] == PATHSEP)
                include_folders[j][strlen(include_folders[j]) - 1] = 0;
        }
        if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
            for (j = 0; j < MAXWARNINGS && muted_warnings[j][0] != 0; j++);
            strncpy(muted_warnings[j], argv[i + 1], sizeof(muted_warnings[j]));
        }
        if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--key") == 0)
            args.privatekey = argv[i + 1];
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--type") == 0)
            args.paatype = argv[i + 1];
    }


    if (args.binarize)
        return cmd_binarize();
    if (args.build)
        return cmd_build();
    if (args.inspect)
        return cmd_inspect();
    if (args.unpack)
        return cmd_unpack();
    if (args.derapify)
        return cmd_derapify();
    if (args.keygen)
        return cmd_keygen();
    if (args.sign)
        return cmd_sign();
    if (args.paa2img)
        return cmd_paa2img();
    if (args.img2paa)
        return cmd_img2paa();


    docopt(2, halp, 1, VERSION);
    return 1;
}
