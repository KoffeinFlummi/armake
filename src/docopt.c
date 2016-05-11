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


#include "docopt.h"


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



const char help_message[] =
"armake\n"
"\n"
"Usage:\n"
"    armake binarize [-f] [-w <wname>] [-i <includefolder>] <source> <target>\n"
"    armake build [-f] [-p] [-w <wname>] [-i <includefolder>] [-x <xlist>] [-k <keyfile>] <source> <target>\n"
"    armake unpack [-f] <source> <target>\n"
"    armake derapify [-f] [-d <indentation>] <source> <target>\n"
"    armake (-h | --help)\n"
"    armake (-v | --version)\n"
"\n"
"Commands:\n"
"    binarize     Binarize a file.\n"
"    build        Pack a folder into a PBO.\n"
"    unpack       Unpack a PBO into a folder.\n"
"    derapify     Derapify a config. You can pass - as the target to print to stdout.\n"
"\n"
"Options:\n"
"    -f --force      Overwrite the target file/folder if it already exists\n"
"    -p --packonly   Don't binarize models, configs etc.\n"
"    -w --warning    Warning to disable (repeatable)\n"
"    -i --include    Folder to search for includes, defaults to CWD (repeatable)\n"
"    -x --exclude    Glob patterns to exclude from PBO (repeatable)\n"
"    -k --key        Keyfile to use for signing the PBO\n"
"    -d --indent     String to use for indentation. \"    \" (4 spaces) by default.\n"
"    -h --help       Show usage information and exit\n"
"    -v --version    Print the version number and exit\n"
"\n"
"Warnings:\n"
"    By default, armake prints all warnings. You can mute trivial warnings\n"
"    using the name that is printed along with them.\n"
"\n"
"    Example: \"-w unquoted-string\" disables warnings about improperly quoted\n"
"             strings.\n"
"";

const char usage_pattern[] =
"Usage:\n"
"    armake binarize [-f] [-w <wname>] [-i <includefolder>] <source> <target>\n"
"    armake build [-f] [-p] [-w <wname>] [-i <includefolder>] [-x <xlist>] [-k <keyfile>] <source> <target>\n"
"    armake unpack [-f] <source> <target>\n"
"    armake derapify [-f] [-d <indentation>] <source> <target>\n"
"    armake (-h | --help)\n"
"    armake (-v | --version)";






/*
 * Tokens object
 */


Tokens tokens_new(int argc, char **argv) {
    Tokens ts = {argc, argv, 0, argv[0]};
    return ts;
}

Tokens* tokens_move(Tokens *ts) {
    if (ts->i < ts->argc) {
        ts->current = ts->argv[++ts->i];
    }
    if (ts->i == ts->argc) {
        ts->current = NULL;
    }
    return ts;
}


/*
 * ARGV parsing functions
 */

int parse_doubledash(Tokens *ts, Elements *elements) {
    //int n_commands = elements->n_commands;
    //int n_arguments = elements->n_arguments;
    //Command *commands = elements->commands;
    //Argument *arguments = elements->arguments;

    // not implemented yet
    // return parsed + [Argument(None, v) for v in tokens]
    return 0;
}

int parse_long(Tokens *ts, Elements *elements) {
    int i;
    int len_prefix;
    int n_options = elements->n_options;
    char *eq = strchr(ts->current, '=');
    Option *option;
    Option *options = elements->options;

    len_prefix = (eq-(ts->current))/sizeof(char);
    for (i=0; i < n_options; i++) {
        option = &options[i];
        if (!strncmp(ts->current, option->olong, len_prefix))
            break;
    }
    if (i == n_options) {
        // TODO '%s is not a unique prefix
        fprintf(stderr, "%s is not recognized\n", ts->current);
        return 1;
    }
    tokens_move(ts);
    if (option->argcount) {
        if (eq == NULL) {
            if (ts->current == NULL) {
                fprintf(stderr, "%s requires argument\n", option->olong);
                return 1;
            }
            option->argument = ts->current;
            tokens_move(ts);
        } else {
            option->argument = eq + 1;
        }
    } else {
        if (eq != NULL) {
            fprintf(stderr, "%s must not have an argument\n", option->olong);
            return 1;
        }
        option->value = true;
    }
    return 0;
}

int parse_shorts(Tokens *ts, Elements *elements) {
    char *raw;
    int i;
    int n_options = elements->n_options;
    Option *option;
    Option *options = elements->options;

    raw = &ts->current[1];
    tokens_move(ts);
    while (raw[0] != '\0') {
        for (i=0; i < n_options; i++) {
            option = &options[i];
            if (option->oshort != NULL && option->oshort[1] == raw[0])
                break;
        }
        if (i == n_options) {
            // TODO -%s is specified ambiguously %d times
            fprintf(stderr, "-%c is not recognized\n", raw[0]);
            return 1;
        }
        raw++;
        if (!option->argcount) {
            option->value = true;
        } else {
            if (raw[0] == '\0') {
                if (ts->current == NULL) {
                    fprintf(stderr, "%s requires argument\n", option->oshort);
                    return 1;
                }
                raw = ts->current;
                tokens_move(ts);
            }
            option->argument = raw;
            break;
        }
    }
    return 0;
}

int parse_argcmd(Tokens *ts, Elements *elements) {
    int i;
    int n_commands = elements->n_commands;
    //int n_arguments = elements->n_arguments;
    Command *command;
    Command *commands = elements->commands;
    //Argument *arguments = elements->arguments;

    for (i=0; i < n_commands; i++) {
        command = &commands[i];
        if (!strcmp(command->name, ts->current)){
            command->value = true;
            tokens_move(ts);
            return 0;
        }
    }
    // not implemented yet, just skip for now
    // parsed.append(Argument(None, tokens.move()))
    /*fprintf(stderr, "! argument '%s' has been ignored\n", ts->current);
    fprintf(stderr, "  '");
    for (i=0; i<ts->argc ; i++)
        fprintf(stderr, "%s ", ts->argv[i]);
    fprintf(stderr, "'\n");*/
    tokens_move(ts);
    return 0;
}

int parse_args(Tokens *ts, Elements *elements) {
    int ret;

    while (ts->current != NULL) {
        if (strcmp(ts->current, "--") == 0) {
            ret = parse_doubledash(ts, elements);
            if (!ret) break;
        } else if (ts->current[0] == '-' && ts->current[1] == '-') {
            ret = parse_long(ts, elements);
        } else if (ts->current[0] == '-' && ts->current[1] != '\0') {
            ret = parse_shorts(ts, elements);
        } else
            ret = parse_argcmd(ts, elements);
        if (ret) return ret;
    }
    return 0;
}

int elems_to_args(Elements *elements, DocoptArgs *args, bool help,
                  const char *version){
    Command *command;
    Argument *argument;
    Option *option;
    int i;

    /* options */
    for (i=0; i < elements->n_options; i++) {
        option = &elements->options[i];
        if (help && option->value && !strcmp(option->olong, "--help")) {
            printf("%s", args->help_message);
            return 1;
        } else if (version && option->value &&
                   !strcmp(option->olong, "--version")) {
            printf("%s\n", version);
            return 1;
        } else if (!strcmp(option->olong, "--exclude")) {
            args->exclude = option->value;
        } else if (!strcmp(option->olong, "--force")) {
            args->force = option->value;
        } else if (!strcmp(option->olong, "--help")) {
            args->help = option->value;
        } else if (!strcmp(option->olong, "--include")) {
            args->include = option->value;
        } else if (!strcmp(option->olong, "--indent")) {
            args->indent = option->value;
        } else if (!strcmp(option->olong, "--key")) {
            args->key = option->value;
        } else if (!strcmp(option->olong, "--packonly")) {
            args->packonly = option->value;
        } else if (!strcmp(option->olong, "--version")) {
            args->version = option->value;
        } else if (!strcmp(option->olong, "--warning")) {
            args->warning = option->value;
        }
    }
    /* commands */
    for (i=0; i < elements->n_commands; i++) {
        command = &elements->commands[i];
        if (!strcmp(command->name, "binarize")) {
            args->binarize = command->value;
        } else if (!strcmp(command->name, "build")) {
            args->build = command->value;
        } else if (!strcmp(command->name, "derapify")) {
            args->derapify = command->value;
        } else if (!strcmp(command->name, "unpack")) {
            args->unpack = command->value;
        }
    }
    /* arguments */
    for (i=0; i < elements->n_arguments; i++) {
        argument = &elements->arguments[i];
        if (!strcmp(argument->name, "<includefolder>")) {
            args->includefolder = argument->value;
        } else if (!strcmp(argument->name, "<indentation>")) {
            args->indentation = argument->value;
        } else if (!strcmp(argument->name, "<keyfile>")) {
            args->keyfile = argument->value;
        } else if (!strcmp(argument->name, "<source>")) {
            args->source = argument->value;
        } else if (!strcmp(argument->name, "<target>")) {
            args->target = argument->value;
        } else if (!strcmp(argument->name, "<wname>")) {
            args->wname = argument->value;
        } else if (!strcmp(argument->name, "<xlist>")) {
            args->xlist = argument->value;
        }
    }
    return 0;
}


/*
 * Main docopt function
 */

DocoptArgs docopt(int argc, char *argv[], bool help, const char *version) {
    DocoptArgs args = {
        0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0,
        0, 0, 0,
        usage_pattern, help_message
    };
    Tokens ts;
    Command commands[] = {
        {"binarize", 0},
        {"build", 0},
        {"derapify", 0},
        {"unpack", 0}
    };
    Argument arguments[] = {
        {"<includefolder>", NULL, NULL},
        {"<indentation>", NULL, NULL},
        {"<keyfile>", NULL, NULL},
        {"<source>", NULL, NULL},
        {"<target>", NULL, NULL},
        {"<wname>", NULL, NULL},
        {"<xlist>", NULL, NULL}
    };
    Option options[] = {
        {"-x", "--exclude", 0, 0, NULL},
        {"-f", "--force", 0, 0, NULL},
        {"-h", "--help", 0, 0, NULL},
        {"-i", "--include", 0, 0, NULL},
        {"-d", "--indent", 0, 0, NULL},
        {"-k", "--key", 0, 0, NULL},
        {"-p", "--packonly", 0, 0, NULL},
        {"-v", "--version", 0, 0, NULL},
        {"-w", "--warning", 0, 0, NULL}
    };
    Elements elements = {4, 7, 9, commands, arguments, options};

    ts = tokens_new(argc, argv);
    if (parse_args(&ts, &elements))
        exit(EXIT_FAILURE);
    if (elems_to_args(&elements, &args, help, version))
        exit(EXIT_SUCCESS);
    return args;
}
