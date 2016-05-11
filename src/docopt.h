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


#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstring>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#endif


#define MAXEXCLUDEFILES 32
#define MAXINCLUDEFOLDERS 32
#define MAXWARNINGS 32


typedef struct {
    /* commands */
    int binarize;
    int build;
    int derapify;
    int unpack;
    /* arguments */
    char *includefolder;
    char *indentation;
    char *keyfile;
    char *source;
    char *target;
    char *wname;
    char *xlist;
    /* options without arguments */
    int exclude;
    int force;
    int help;
    int include;
    int indent;
    int key;
    int packonly;
    int version;
    int warning;
    /* special */
    const char *usage_pattern;
    const char *help_message;
} DocoptArgs;

typedef struct {
    const char *name;
    bool value;
} Command;

typedef struct {
    const char *name;
    char *value;
    char **array;
} Argument;

typedef struct {
    const char *oshort;
    const char *olong;
    bool argcount;
    bool value;
    char *argument;
} Option;

typedef struct {
    int n_commands;
    int n_arguments;
    int n_options;
    Command *commands;
    Argument *arguments;
    Option *options;
} Elements;

typedef struct Tokens {
    int argc;
    char **argv;
    int i;
    char *current;
} Tokens;


DocoptArgs args;
char exclude_files[MAXEXCLUDEFILES][512];
char include_folders[MAXINCLUDEFOLDERS][512];
char muted_warnings[MAXWARNINGS][512];


Tokens tokens_new(int argc, char **argv);

Tokens* tokens_move(Tokens *ts);

int parse_doubledash(Tokens *ts, Elements *elements);

int parse_long(Tokens *ts, Elements *elements);

int parse_shorts(Tokens *ts, Elements *elements);

int parse_argcmd(Tokens *ts, Elements *elements);

int parse_args(Tokens *ts, Elements *elements);

int elems_to_args(Elements *elements, DocoptArgs *args, bool help,
                  const char *version);

DocoptArgs docopt(int argc, char *argv[], bool help, const char *version);

