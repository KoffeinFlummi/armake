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


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <dirent.h>
#include <unistd.h>
#endif

#include "filesystem.h"


#ifdef _WIN32
size_t getline(char **lineptr, size_t *n, FILE *stream) {
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL)
        return -1;
    if (stream == NULL)
        return -1;
    if (n == NULL)
        return -1;

    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF)
        return -1;

    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL)
            return -1;
        size = 128;
    }
    p = bufptr;
    while(c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = realloc(bufptr, size);
            if (bufptr == NULL)
                return -1;
        }

        *p++ = c;

        if (c == '\n')
            break;

        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}
#endif


int get_temp_name(char *target, char *suffix) {
#ifdef _WIN32
    if (!GetTempFileName(".", "amk", 0, target)) { return 1; }
    strcat(target, suffix);
#else
    strcpy(target, "amk_XXXXXX");
    strcat(target, suffix);
    return mkstemps(target, strlen(suffix)) == -1;
#endif
}


int create_folder(char *path) {
    /*
     * Create the given folder. Returns -2 if the directory already exists,
     * -1 on error and 0 on success.
     */

#ifdef _WIN32

    if (!CreateDirectory(path, NULL)) {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
            return -2;
        else
            return -1;
    }

    return 0;

#else

    struct stat st = {0};

    if (stat(path, &st) != -1)
        return -2;

    return mkdir(path, 0755);

#endif
}


int create_folders(char *path) {
    /*
     * Recursively create all folders for the given path. Returns -1 on
     * failure and 0 on success.
     */

    char tmp[2048];
    char *p = NULL;
    int success;
    size_t len;

    tmp[0] = 0;
    strcat(tmp, path);
    len = strlen(tmp);

    if (tmp[len - 1] == PATHSEP)
        tmp[len - 1] = 0;

    for (p = tmp + 1; *p; p++) {
        if (*p == PATHSEP && *(p-1) != ':') {
            *p = 0;
            success = create_folder(tmp);
            if (success != -2 && success != 0)
                return success;
            *p = PATHSEP;
        }
    }

    success = create_folder(tmp);
    if (success != -2 && success != 0)
        return success;

    return 0;
}


int create_temp_folder(char *addon, char *temp_folder, size_t bufsize) {
    /*
     * Create a temp folder for the given addon in the proper place
     * depending on the operating system. Returns -1 on failure and 0
     * on success.
     */

    char temp[2048] = TEMPPATH;
    char addon_sanitized[2048];
    int i;

#ifdef _WIN32
    temp[0] = 0;
    GetTempPath(sizeof(temp), temp);
    strcat(temp, "armake\\");
#endif

    temp[strlen(temp) - 1] = 0;

    for (i = 0; i <= strlen(addon); i++)
        addon_sanitized[i] = (addon[i] == '\\' || addon[i] == '/') ? '_' : addon[i];

    // find a free one
    for (i = 0; i < 1024; i++) {
        snprintf(temp_folder, bufsize, "%s_%s_%i%c", temp, addon_sanitized, i, PATHSEP);
        if (access(temp_folder, F_OK) == -1)
            break;
    }

    if (i == 1024)
        return -1;

    return create_folders(temp_folder);
}


int remove_file(char *path) {
    /*
     * Remove a file. Returns 0 on success and 1 on failure.
     */

#ifdef _WIN32
    return !DeleteFile(path);
#else
    return (remove(path) * -1);
#endif
}


int remove_folder(char *folder) {
    /*
     * Recursively removes a folder tree. Returns a negative integer on
     * failure and 0 on success.
     */

#ifdef _WIN32

    // MASSIVE @todo
    char cmd[512];
    sprintf(cmd, "rmdir %s /s /q", folder);
    if (system(cmd))
        return -1;

#else

    // ALSO MASSIVE @todo
    char cmd[512];
    sprintf(cmd, "rm -rf %s", folder);
    if (system(cmd))
        return -1;

#endif

    return 0;
}


int copy_file(char *source, char *target) {
    /*
     * Copy the file from the source to the target. Overwrites if the target
     * already exists.
     * Returns a negative integer on failure and 0 on success.
     */

    // Create the containing folder
    char containing[strlen(target) + 1];
    int lastsep = 0;
    int i;
    for (i = 0; i < strlen(target); i++) {
        if (target[i] == PATHSEP)
            lastsep = i;
    }
    strcpy(containing, target);
    containing[lastsep] = 0;

    if (create_folders(containing))
        return -1;

#ifdef _WIN32

    if (!CopyFile(source, target, 0))
        return -2;

#else

    int f_target, f_source;
    char buf[4096];
    ssize_t nread;

    f_source = open(source, O_RDONLY);
    if (f_source < 0)
        return -2;

    f_target = open(target, O_WRONLY | O_CREAT, 0666);
    if (f_target < 0) {
        close(f_source);
        if (f_target >= 0)
            close(f_target);
        return -3;
    }

    while (nread = read(f_source, buf, sizeof buf), nread > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(f_target, out_ptr, nread);

            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            } else if (errno != EINTR) {
                close(f_source);
                if (f_target >= 0)
                    close(f_target);
                return -4;
            }
        } while (nread > 0);
    }

    close(f_source);
    if (close(f_target) < 0)
        return -5;

#endif

    return 0;
}


#ifndef _WIN32
int alphasort_ci(const struct dirent **a, const struct dirent **b) {
    /*
     * A case insensitive version of alphasort.
     */

    int i;
    char a_name[512];
    char b_name[512];

    strncpy(a_name, (*a)->d_name, sizeof(a_name));
    strncpy(b_name, (*b)->d_name, sizeof(b_name));

    for (i = 0; i < strlen(a_name); i++) {
        if (a_name[i] >= 'A' && a_name[i] <= 'Z')
            a_name[i] = a_name[i] - ('A' - 'a');
    }

    for (i = 0; i < strlen(b_name); i++) {
        if (b_name[i] >= 'A' && b_name[i] <= 'Z')
            b_name[i] = b_name[i] - ('A' - 'a');
    }

    return strcoll(a_name, b_name);
}
#endif


int traverse_directory_recursive(char *root, char *cwd, int (*callback)(char *, char *, char *), char *third_arg) {
    /*
     * Recursive helper function for directory traversal.
     */

#ifdef _WIN32

    WIN32_FIND_DATA file;
    HANDLE handle = NULL;
    char mask[2048];
    int success;

    if (cwd[strlen(cwd) - 1] == '\\')
        cwd[strlen(cwd) - 1] = 0;

    GetFullPathName(cwd, 2048, mask, NULL);
    sprintf(mask, "%s\\*", mask);

    handle = FindFirstFile(mask, &file);
    if (handle == INVALID_HANDLE_VALUE)
        return 1;

    do {
        if (strcmp(file.cFileName, ".") == 0 || strcmp(file.cFileName, "..") == 0)
            continue;

        sprintf(mask, "%s\\%s", cwd, file.cFileName);
        if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            traverse_directory_recursive(root, mask, callback, third_arg);
        } else {
            success = callback(root, mask, third_arg);
            if (success)
                return success;
        }
    } while (FindNextFile(handle, &file));

    FindClose(handle);

    return 0;

#else

    struct dirent **namelist;
    char next[2048];
    int i;
    int n;
    int success;

    n = scandir(cwd, &namelist, NULL, alphasort_ci);
    if (n < 0)
        return 1;

    success = 0;
    for (i = 0; i < n; i++) {
        if (strcmp(namelist[i]->d_name, "..") == 0 ||
                strcmp(namelist[i]->d_name, ".") == 0)
            continue;

        strcpy(next, cwd);
        strcat(next, "/");
        strcat(next, namelist[i]->d_name);

        switch (namelist[i]->d_type) {
            case DT_DIR:
                success = traverse_directory_recursive(root, next, callback, third_arg);
                if (success)
                    goto cleanup;
                break;

            case DT_REG:
                success = callback(root, next, third_arg);
                if (success)
                    goto cleanup;
                break;
        }
    }

cleanup:
    for (i = 0; i < n; i++)
        free(namelist[i]);
    free(namelist);

    return success;

#endif
}


int traverse_directory(char *root, int (*callback)(char *, char *, char *), char *third_arg) {
    /*
     * Traverse the given path and call the callback with the root folder as
     * the first, the current file path as the second, and the given third
     * arg as the third argument.
     *
     * The callback should return 0 success and any negative integer on
     * failure.
     *
     * This function returns 0 on success, a positive integer on a traversal
     * error and the last callback return value should the callback fail.
     */

    return traverse_directory_recursive(root, root, callback, third_arg);
}


int copy_callback(char *source_root, char *source, char *target_root) {
    // Remove trailing path seperators
    if (source_root[strlen(source_root) - 1] == PATHSEP)
        source_root[strlen(source_root) - 1] = 0;
    if (target_root[strlen(target_root) - 1] == PATHSEP)
        target_root[strlen(target_root) - 1] = 0;

    if (strstr(source, source_root) != source)
        return -1;

    char target[strlen(source) + strlen(target_root) + 1]; // assume worst case
    target[0] = 0;
    strcat(target, target_root);
    strcat(target, source + strlen(source_root));

    return copy_file(source, target);
}


int copy_directory(char *source, char *target) {
    /*
     * Copy the entire directory given with source to the target folder.
     * Returns 0 on success and a non-zero integer on failure.
     */

    // Remove trailing path seperators
    if (source[strlen(source) - 1] == PATHSEP)
        source[strlen(source) - 1] = 0;
    if (target[strlen(target) - 1] == PATHSEP)
        target[strlen(target) - 1] = 0;

    return traverse_directory(source, copy_callback, target);
}
