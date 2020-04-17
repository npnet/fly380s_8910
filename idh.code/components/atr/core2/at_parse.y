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
#include "at_parse.h"

typedef void *yyscan_t;
extern int at_parse_lex(yyscan_t yyscanner);
extern int atParseStartCmdNone(yyscan_t yyscanner);
extern int atParseStartCmd(yyscan_t yyscanner);
extern int atParseSetCmdType(yyscan_t yyscanner, uint8_t cmdtype);
extern int atParsePushRawText(yyscan_t env);
extern int atParsePushEmpty(yyscan_t env);
extern void *at_parse_alloc(unsigned size, void *);
extern void at_parse_free(void *ptr, void *);
extern void at_parse_begin_initial(yyscan_t yyscanner);

#define at_parse_error(env, msg)
#define malloc(size) at_parse_alloc(size, scanner)
#define free(ptr) at_parse_free(ptr, scanner);

#define RETURN_IF_FAIL(n) do { int result = (n); if (result != 0) return result; } while (0)

%}
%name-prefix    "at_parse_"
%defines        "at_parse.y.h"
%output         "at_parse.y.c"
%lex-param      {yyscan_t scanner}
%parse-param    {yyscan_t scanner}

%token  AT_CMD_PREFIX
%token  AT_CMD_DIVIDER
%token  AT_CMD_END
%token  AT_BASIC_CMD AT_S_CMD AT_EXT_CMD AT_D_CMD
%token  NUMBER STRING DSTRING RAWTEXT
%union  { int dummy; }

%start  AT_cmd_line
%%

AT_cmd_line :   AT_CMD_PREFIX AT_cmds AT_CMD_END
            |   AT_CMD_PREFIX AT_CMD_END { RETURN_IF_FAIL(atParseStartCmdNone(scanner)); }
            ;

AT_cmds     :   AT_cmds AT_divider AT_compound_cmd
            |   AT_cmds AT_divider AT_ext_cmd
            |   AT_compound_cmd AT_ext_cmd
            |   AT_compound_cmd
            |   AT_ext_cmd
            ;

AT_compound_cmd :   AT_compound_cmd AT_basic_cmd
                |   AT_compound_cmd AT_s_cmd
                |   AT_compound_cmd AT_d_cmd
                |   AT_basic_cmd
                |   AT_s_cmd
                |   AT_d_cmd
                ;

AT_basic_cmd    :   AT_basic_name '?' { RETURN_IF_FAIL(atParseSetCmdType(scanner, AT_CMD_READ)); }
                |   AT_basic_name { RETURN_IF_FAIL(atParseSetCmdType(scanner, AT_CMD_EXE)); }
                |   AT_basic_name { RETURN_IF_FAIL(atParseSetCmdType(scanner, AT_CMD_EXE)); } NUMBER { RETURN_IF_FAIL(atParsePushRawText(scanner)); }
                |   AT_basic_name '=' { RETURN_IF_FAIL(atParseSetCmdType(scanner, AT_CMD_SET)); } NUMBER { RETURN_IF_FAIL(atParsePushRawText(scanner)); }
                |   AT_basic_name '=' '?' { RETURN_IF_FAIL(atParseSetCmdType(scanner, AT_CMD_TEST)); }
                ;

AT_s_cmd        :   AT_s_name '?' { RETURN_IF_FAIL(atParseSetCmdType(scanner, AT_CMD_READ)); }
                |   AT_s_name '=' { RETURN_IF_FAIL(atParseSetCmdType(scanner, AT_CMD_SET)); }
                |   AT_s_name '=' { RETURN_IF_FAIL(atParseSetCmdType(scanner, AT_CMD_SET)); } NUMBER { RETURN_IF_FAIL(atParsePushRawText(scanner)); }
                |   AT_s_name '=' '?' { RETURN_IF_FAIL(atParseSetCmdType(scanner, AT_CMD_TEST)); }
                ;

AT_d_cmd        :   AT_d_name { RETURN_IF_FAIL(atParseSetCmdType(scanner, AT_CMD_EXE)); } DSTRING { RETURN_IF_FAIL(atParsePushRawText(scanner)); }
                ;


AT_ext_cmd      :   AT_ext_name { RETURN_IF_FAIL(atParseSetCmdType(scanner, AT_CMD_EXE)); }
                |   AT_ext_name '?' { RETURN_IF_FAIL(atParseSetCmdType(scanner, AT_CMD_READ)); }
                |   AT_ext_name '=' '?' { RETURN_IF_FAIL(atParseSetCmdType(scanner, AT_CMD_TEST)); }
                |   AT_ext_name '=' { RETURN_IF_FAIL(atParseSetCmdType(scanner, AT_CMD_SET)); } ExtParams
                ;

AT_basic_name   :   AT_BASIC_CMD { RETURN_IF_FAIL(atParseStartCmd(scanner)); } ;
AT_s_name       :   AT_S_CMD { RETURN_IF_FAIL(atParseStartCmd(scanner)); } ;
AT_d_name       :   AT_D_CMD { RETURN_IF_FAIL(atParseStartCmd(scanner)); } ;
AT_ext_name     :   AT_EXT_CMD { RETURN_IF_FAIL(atParseStartCmd(scanner)); } ;

AT_divider      :   AT_CMD_DIVIDER { at_parse_begin_initial(scanner); }

ExtParams       :   ExtParam
                |   ExtParams ',' ExtParam
                ;

ExtParam        :   STRING { RETURN_IF_FAIL(atParsePushRawText(scanner)); }
                |   RAWTEXT { RETURN_IF_FAIL(atParsePushRawText(scanner)); }
                |   { RETURN_IF_FAIL(atParsePushEmpty(scanner)); }
                ;

%%
