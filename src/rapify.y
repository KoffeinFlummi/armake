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

%{
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "utils.h"
#include "filesystem.h"
#include "preprocess.h"
#include "rapify.h"

#define YYDEBUG 0
#define YYERROR_VERBOSE 1

extern int yylex(struct class **result, struct lineref *lineref);
extern int yyparse();
extern FILE* yyin;
extern int yylineno;

void yyerror(struct class **result, struct lineref *lineref, const char* s);
%}

%union {
    struct definitions* definitions_value;
    struct class *class_value;
    struct variable *variable_value;
    struct expression *expression_value;
    int32_t int_value;
    float float_value;
    char *string_value;
}

%token<string_value> T_NAME
%token<int_value> T_INT
%token<float_value> T_FLOAT
%token<string_value> T_STRING
%token T_CLASS T_DELETE
%token T_SEMICOLON T_COLON T_COMMA T_EQUALS T_PLUS
%token T_LBRACE T_RBRACE
%token T_LBRACKET T_RBRACKET

%type<definitions_value> definitions
%type<class_value> class
%type<variable_value> variable
%type<expression_value> expression expressions

%start start

%param {struct class **result} {struct lineref *lineref}
%locations

%%
start: definitions { *result = new_class(NULL, NULL, $1, false); }

definitions:  /* empty */ { $$ = new_definitions(); }
            | definitions class { $$ = add_definition($1, TYPE_CLASS, $2); }
            | definitions variable { $$ = add_definition($1, TYPE_VAR, $2); }
;

class:        T_CLASS T_NAME T_LBRACE definitions T_RBRACE T_SEMICOLON { $$ = new_class($2, NULL, $4, false); }
            | T_CLASS T_NAME T_COLON T_NAME T_LBRACE definitions T_RBRACE T_SEMICOLON { $$ = new_class($2, $4, $6, false); }
            | T_CLASS T_NAME T_SEMICOLON { $$ = new_class($2, NULL, NULL, false); }
            | T_CLASS T_NAME T_COLON T_NAME T_SEMICOLON { $$ = new_class($2, $4, 0, false); }
            | T_DELETE T_NAME T_SEMICOLON { $$ = new_class($2, NULL, NULL, true); }
;

variable:     T_NAME T_EQUALS expression T_SEMICOLON { $$ = new_variable(TYPE_VAR, $1, $3); }
            | T_NAME T_LBRACKET T_RBRACKET T_EQUALS expression T_SEMICOLON { $$ = new_variable(TYPE_ARRAY, $1, $5); }
            | T_NAME T_LBRACKET T_RBRACKET T_PLUS T_EQUALS expression T_SEMICOLON { $$ = new_variable(TYPE_ARRAY_EXPANSION, $1, $6); }
;

expression:   T_INT { $$ = new_expression(TYPE_INT, &$1); }
            | T_FLOAT { $$ = new_expression(TYPE_FLOAT, &$1); }
            | T_STRING { $$ = new_expression(TYPE_STRING, $1); }
            | T_LBRACE expressions T_RBRACE { $$ = new_expression(TYPE_ARRAY, $2); }
            | T_LBRACE expressions T_COMMA T_RBRACE { $$ = new_expression(TYPE_ARRAY, $2); }
            | T_LBRACE T_RBRACE { $$ = new_expression(TYPE_ARRAY, NULL); }
;

expressions:  expression { $$ = $1; }
            | expressions T_COMMA expression { $$ = add_expression($1, $3); }
;
%%

struct class *parse_file(FILE *f, struct lineref *lineref) {
    struct class *result;

    yylineno = 0;
    yyin = f;

#if YYDEBUG == 1
    yydebug = 1;
#endif

    do { 
        if (yyparse(&result, lineref)) {
            return NULL;
        }
    } while(!feof(yyin));

    return result;
}

void yyerror(struct class **result, struct lineref *lineref,  const char* s) {
    int line = 0;
    char *buffer = NULL;
    size_t buffsize;

    fseek(yyin, 0, SEEK_SET);
    while (line < yylloc.first_line) {
        if (buffer != NULL)
            free(buffer);

        buffer = NULL;
        buffsize = 0;
        getline(&buffer, &buffsize, yyin);

        line++;
    }

    fprintf(stderr, "In file %s:%i: ", lineref->file_names[lineref->file_index[yylloc.first_line]], lineref->line_number[yylloc.first_line]);
    errorf("%s\n", s);

    fprintf(stderr, " %s", buffer);
}
