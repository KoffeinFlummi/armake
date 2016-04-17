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

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#else
#include <sys/sendfile.h>
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

    char tmp[256];
    char *p = NULL;
    int success;
    size_t len;

    tmp[0] = 0;
    strcat(tmp, path);
    len = strlen(tmp);

    if (tmp[len - 1] == PATHSEP)
        tmp[len - 1] = 0;

    for (p = tmp + 1; *p; p++) {
        if (*p == PATHSEP) {
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
#ifdef _WIN32
    temp[0] = 0;
    GetTempPath(sizeof(temp), temp);
    strcat(temp, "armake\\");
#endif

    if (strlen(temp) + strlen(addon) + 1 > bufsize)
        return -1;

    temp_folder[0] = 0;
    strcat(temp_folder, temp);
    strcat(temp_folder, addon);

    int success = create_folders(temp_folder);

    if (success == -2) // already exists
        return 0;

    return success;
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


int remove_temp_folder() {
    char tempfolder[2048] = TEMPPATH;
#ifdef _WIN32
    GetTempPath(sizeof(tempfolder), tempfolder);
    strcat(tempfolder, "armake\\");
#endif
    return remove_folder(tempfolder);
}


int copy_file(char *source, char *target) {
    /*
     * Copy the file from the source to the target. Overwrites if the target
     * already exists.
     * Returns a negative integer on failure and 0 on success.
     */

    // Create the containing folder
    char containing[strlen(target)];
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
    int result;
    struct dirent *a_temp;
    struct dirent *b_temp;

    a_temp = (struct dirent *)malloc(sizeof(struct dirent));
    b_temp = (struct dirent *)malloc(sizeof(struct dirent));

    memcpy(a_temp, *a, sizeof(struct dirent));
    memcpy(b_temp, *b, sizeof(struct dirent));

    for (i = 0; i < strlen(a_temp->d_name); i++) {
        if (a_temp->d_name[i] >= 'A' && a_temp->d_name[i] <= 'Z')
            a_temp->d_name[i] = a_temp->d_name[i] - ('A' - 'a');
    }

    for (i = 0; i < strlen(b_temp->d_name); i++) {
        if (b_temp->d_name[i] >= 'A' && b_temp->d_name[i] <= 'Z')
            b_temp->d_name[i] = b_temp->d_name[i] - ('A' - 'a');
    }

    result = alphasort((const struct dirent **)&a_temp, (const struct dirent **)&b_temp);

    free(a_temp);
    free(b_temp);

    return result;
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

#else

    struct dirent **namelist;
    char next[2048];
    int i;
    int n;
    int success;
    
    n = scandir(cwd, &namelist, NULL, alphasort_ci);
    if (n < 0)
        return 1;

    for (i = 0; i < n; i++) {
        if (strcmp(namelist[i]->d_name, "..") == 0 ||
                strcmp(namelist[i]->d_name, ".") == 0) {
            free(namelist[i]);
            continue;
        }

        strcpy(next, cwd);
        strcat(next, "/");
        strcat(next, namelist[i]->d_name);

        switch (namelist[i]->d_type) {
            case DT_DIR:
                success = traverse_directory_recursive(root, next, callback, third_arg);
                if (success)
                    return success;
                break;

            case DT_REG:
                success = callback(root, next, third_arg);
                if (success)
                    return success;
                break;
        }

        free(namelist[i]);
    }

    free(namelist);

#endif
    
     return 0;
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
