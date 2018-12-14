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

bool allow_val = false;
bool allow_arr = false;
bool last_was_class = false;
bool tmp;

#define YY_NO_INPUT
#define YY_NO_UNPUT

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno;

#define RESET_VARS \
    allow_val = false; \
    allow_arr = false; \
    last_was_class = false;
%}

%%

[ \t] {}
\n {}

";" {RESET_VARS; return T_SEMICOLON;}
":" {tmp = last_was_class; RESET_VARS; last_was_class = tmp; return T_COLON;}
"," {RESET_VARS; allow_arr = true; return T_COMMA;}
"+" {RESET_VARS; return T_PLUS;}
"=" {RESET_VARS; allow_val = true; return T_EQUALS;}
"]" {RESET_VARS; return T_RBRACKET;}
"[" {RESET_VARS; return T_LBRACKET;}
"}" {RESET_VARS; return T_RBRACE;}
"{" {tmp = !last_was_class; RESET_VARS; allow_arr = tmp; return T_LBRACE;}

"class" {RESET_VARS; last_was_class = true; return T_CLASS;}
"delete" {RESET_VARS; return T_DELETE;}

\s*[-+]?[0-9]+ {
    if (!allow_val && !allow_arr)
        REJECT;
    RESET_VARS;
    yylval.int_value = atol(yytext);
    return T_INT;
}

\s*[-+]?0x[0-9]+ {
    RESET_VARS;
    yylval.int_value = strtol(yytext, NULL, 16);
    return T_INT;
}

\s*[-+]?[0-9]*\.[0-9]+ {
    RESET_VARS;
    yylval.float_value = atof(yytext);
    return T_FLOAT;
}

\s*[-+]?([0-9]*\.)?[0-9]+[eE][-+]?[0-9]+ {
    RESET_VARS;
    yylval.string_value = (char *)safe_malloc(yyleng + 1);
    strcpy(yylval.string_value, yytext);
    return T_STRING;
}

\"(\"\"|[^"])*?\"    {
    RESET_VARS;
    yylval.string_value = (char *)safe_malloc(yyleng + 1);
    strcpy(yylval.string_value, yytext);
    unescape_string(yylval.string_value, yyleng + 1);
    return T_STRING;
}

'(''|[^'])*?' {
    RESET_VARS;
    yylval.string_value = (char *)safe_malloc(yyleng + 1);
    strcpy(yylval.string_value, yytext);
    unescape_string(yylval.string_value, yyleng + 1);
    return T_STRING;
}

[^;,{"' \t\n][^;{\n]*/[ \t\n]*; {
    if (!allow_val)
        REJECT;

    trim(yytext, yyleng + 1);

    char *endptr;
    strtof(yytext, &endptr);
    if (*endptr == 0)
        REJECT;

    lnwarningf(lineref->file_names[lineref->file_index[yylineno]], lineref->line_number[yylineno],
            "unquoted-string", "String \"%s\" is not quoted properly.\n", yytext);

    RESET_VARS;
    yylval.string_value = (char *)safe_malloc(yyleng + 1);
    strcpy(yylval.string_value, yytext);
    trim(yylval.string_value, yyleng + 1);
    return T_STRING;
}

[^;,{"'} \t\n][^;,{}\n]*/[ \t\n]*[,}] {
    if (!allow_arr)
        REJECT;

    trim(yytext, yyleng + 1);

    char *endptr;
    strtof(yytext, &endptr);
    if (*endptr == 0)
        REJECT;

    lnwarningf(lineref->file_names[lineref->file_index[yylineno]], lineref->line_number[yylineno],
            "unquoted-string", "String \"%s\" is not quoted properly.\n", yytext);

    RESET_VARS;
    yylval.string_value = (char *)safe_malloc(yyleng + 1);
    strcpy(yylval.string_value, yytext);
    trim(yylval.string_value, yyleng + 1);
    return T_STRING;
}

[a-zA-Z0-9_]+ {
    if (allow_arr || allow_val)
        REJECT;

    tmp = last_was_class;
    RESET_VARS;
    last_was_class = tmp;

    yylval.string_value = (char *)safe_malloc(yyleng + 1);
    strcpy(yylval.string_value, yytext);
    return T_NAME;
}

. {}

%%
