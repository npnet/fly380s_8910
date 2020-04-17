/* Copyright (C) 2017 RDA Technologies Limited and/or its affiliates("RDA").
* All rights reserved.
*
* This software is supplied "AS IS" without any warranties.
* RDA assumes no responsibility or liability for the use of the software,
* conveys no license or title under any patent, copyright, or mask work
* right to the product. RDA reserves the right to make changes in the
* software without notification.  RDA also make no representation or
* warranty that such application will be suitable for the specified use
* without further testing or modification.
*/

%{
#include "at_parse.y.h"

#define YY_INPUT(buf, result, max_size) result = YY_NULL
#define YY_FATAL_ERROR(msg)
#define YY_NO_FATAL_ERROR

%}
%option     8bit
%option     reentrant
%option     outfile="at_parse.lex.c"
%option     header-file="at_parse.lex.h"
%option     case-insensitive
%option     noyylineno
%option     batch
%option     noyywrap
%option     nounistd
%option     nodefault
%option     nounput
%option     noinput
%option     prefix="at_parse_"
%option     noyyalloc
%option     noyyrealloc
%option     noyyfree

%x ST_EXT_CMD
%x ST_BASIC_CMD
%x ST_D_CMD

SPACE       [ \t]+
NUMBER      [+-]?[0-9]+
DSTRING     ([0-9*#+ABC,TP!W@]+|>{STRING}?{NUMBER}?)[IG]*;?
STRING      \"([^"\r]|\\\")*\"
RAWTEXT     [^ \t,;"\r=\?][^ \t,;"\r]*

EXT_CMD     [+*^%$][^ \t\r,;=?]+
S_CMD       S[0-9]+
BASIC_CMD   &?[A-Z]

%%

"AT"        { return AT_CMD_PREFIX; }
<*>";"      { return AT_CMD_DIVIDER; }
<*>"\r"     { return AT_CMD_END; }

<INITIAL,ST_BASIC_CMD>{S_CMD}       { BEGIN(ST_BASIC_CMD); return AT_S_CMD; }
<INITIAL,ST_BASIC_CMD>"DL"          { BEGIN(ST_BASIC_CMD); return AT_BASIC_CMD; }
<INITIAL,ST_BASIC_CMD>"D"           { BEGIN(ST_D_CMD); return AT_D_CMD; }
<INITIAL,ST_BASIC_CMD>{BASIC_CMD}   { BEGIN(ST_BASIC_CMD); return AT_BASIC_CMD; }
<INITIAL,ST_BASIC_CMD>{EXT_CMD}     { BEGIN(ST_EXT_CMD); return AT_EXT_CMD; }

<ST_BASIC_CMD>{NUMBER}  { BEGIN(INITIAL); return NUMBER; }

<ST_EXT_CMD>{STRING}    { return STRING; }
<ST_EXT_CMD>{RAWTEXT}   { return RAWTEXT; }

<ST_D_CMD>{DSTRING}     { BEGIN(INITIAL); return DSTRING; }

<<EOF>>     { return 0; }
<*>{SPACE}  { }
<*>(.|\n)   { return yytext[0]; }
%%

void at_parse_begin_initial(yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    BEGIN(INITIAL);
}
