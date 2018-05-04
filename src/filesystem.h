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

#pragma once


#ifdef _WIN32
#define PATHSEP '\\'
#define PATHSEP_STR "\\"
#define TEMPPATH "C:\\Windows\\Temp\\armake\\"
#else
#define PATHSEP '/'
#define PATHSEP_STR "/"
#define TEMPPATH "/tmp/armake/"
#endif


#ifdef _WIN32
ssize_t getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp);

ssize_t getline(char **buf, size_t *bufsiz, FILE *fp);
#endif

int get_temp_name(char *target, char *suffix);

int create_folder(char *path);

int create_folders(char *path);

int create_temp_folder(char *addon, char *temp_folder, size_t bufsize);

int remove_file(char *path);

int remove_folder(char *folder);

int copy_file(char *source, char *target);

int traverse_directory(char *root, int (*callback)(char *, char *, char *),
    char *third_arg);

int copy_directory(char *source, char *target);
