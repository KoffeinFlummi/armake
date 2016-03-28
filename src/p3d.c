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

#include "docopt.h"
#include "filesystem.h"
#include "utils.h"
#include "p3d.h"


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

    // Write ODOL to temp file
    f_source = fopen(source, "r");
    if (!f_source) {
        printf("Failed to open source file.\n");
        return 2;
    }

    // Check if the file is actually MLOD
    fgets(buffer, sizeof(buffer), f_source);
    if (strncmp(buffer, "MLOD", 4) != 0) {
        printf("Source file is not MLOD.\n");
        return 3;
    }

    // Read num of lods
    fseek(f_source, 8, SEEK_SET);
    fread(&num_lods, 4, 1, f_source);
    printf("%i LODs.\n", num_lods);

    // Write header
    fwrite("ODOL", 4, 1, f_target);
    fwrite("\x46\0\0\0", 4, 1, f_target); // version 70
    fwrite("\0", 1, 1, f_target); // prefix
    fwrite(&num_lods, 4, 1, f_target);

    // Write model info

    // Write animations (@todo)
    fwrite("\0", 1, 1, f_target); // nope

    // Write LODs (@todo)

    // Write PhysX (@todo)
    //fwrite("\0\0\0\0", 4, 1, f_target);

    // Write temp to target
    fclose(f_source);
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
