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

%option noyywrap
%option yylineno
%option nodebug

%{
#define YY_DECL int yylex(struct class **result, struct lineref *lineref)

#include <stdio.h>
#include <stdbool.h>
#include "utils.h"
#include "rapify.h"
#include "rapify.tab.h"

bool allow_unquoted_string = false;
bool allow_name = false;
bool last_was_name = false;

#define YY_NO_INPUT
#define YY_NO_UNPUT

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno;

#define RESET_VARS \
    allow_unquoted_string = false; \
    allow_name = false; \
    last_was_name = false
%}

%%

[ \t] {}
\n {}

";" {RESET_VARS; allow_name = true; return T_SEMICOLON;}
":" {RESET_VARS; allow_name = true; return T_COLON;}
"," {RESET_VARS; allow_unquoted_string = true; return T_COMMA;}
"+" {RESET_VARS; return T_PLUS;}
"=" {RESET_VARS; allow_unquoted_string = true; return T_EQUALS;}
"]" {RESET_VARS; return T_RBRACKET;}
"[" {RESET_VARS; return T_LBRACKET;}
"}" {RESET_VARS; return T_RBRACE;}
"{" {
    bool is_class = last_was_name;
    RESET_VARS;
    allow_unquoted_string = !is_class;
    allow_name = is_class;
    return T_LBRACE;
}

"class" {
    RESET_VARS;
    allow_name = true;
    return T_CLASS;
}

"delete" {
    RESET_VARS; 
    allow_name = true;
    return T_DELETE;
}

-?[0-9]+ {
    RESET_VARS;
    yylval.int_value = atol(yytext);
    return T_INT;
}

-?[0-9]+\.[0-9]+ {
    RESET_VARS;
    yylval.float_value = atof(yytext);
    return T_FLOAT;
}

\"(\\.|\"\"|[^"])*\"    {
    RESET_VARS;
    yylval.string_value = (char *)malloc(yyleng + 1);
    strcpy(yylval.string_value, yytext);
    unescape_string(yylval.string_value, yyleng + 1);
    return T_STRING;
}

'(\\.|''|[^'])*' {
    RESET_VARS;
    yylval.string_value = (char *)malloc(yyleng + 1);
    strcpy(yylval.string_value, yytext);
    unescape_string(yylval.string_value, yyleng + 1);
    return T_STRING;
}

([^"class""delete" \t\n\\\"';,=\{\}]([^\n\\\"';,=\{\}]*[^ \t\n\\\"';,=\{\}])?) {
    if (!allow_unquoted_string) {
        REJECT;
    }

    if (!warning_muted("unquoted-string"))
        fprintf(stderr, "In file %s:%i: ", lineref->file_names[lineref->file_index[yylineno]], yylineno);
    nwarningf("unquoted-string", "String \"%s\" is not quoted properly.\n", yytext);

    RESET_VARS;
    yylval.string_value = (char *)malloc(yyleng + 1);
    strcpy(yylval.string_value, yytext);
    return T_STRING;
}

[a-zA-Z][a-zA-Z0-9_]* {
    if (!allow_name) {
        REJECT;
    }

    RESET_VARS;
    last_was_name = true;
    yylval.string_value = (char *)malloc(yyleng + 1);
    strcpy(yylval.string_value, yytext);
    return T_NAME;
}

%%
