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

#include "at_parse.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int at_parse_lex_init_extra(void *extra, void **scanner);
void *at_parse__scan_bytes(const char *bytes, unsigned len, void *scanner);
int at_parse_lex_destroy(void *scanner);
int at_parse_parse(void *scanner);
extern unsigned at_parse_get_leng(void *scanner);
char *at_parse_get_text(void *scanner);
void *at_parse_get_extra(void *yyscanner);

void *at_parse_alloc(unsigned size, void *scanner)
{
    void *ptr = malloc(size);
    printf("AT CMD malloc %u %p\n", size, ptr);
    return ptr;
}

void *at_parse_realloc(void *ptr, unsigned size, void *scanner)
{
    void *nptr = realloc(ptr, size);
    printf("AT CMD realloc %u %p -> %p\n", size, ptr, nptr);
    return nptr;
}

void at_parse_free(void *ptr, void *scanner)
{
    printf("AT CMD free %p\n", ptr);
    free(ptr);
}

static int atParseStartCmdText(char *text, unsigned length)
{
    printf("AT CMD parse start cmd len=%d: %s\n", length, text);
    return 0;
}

int atParseStartCmd(void *scanner)
{
    return atParseStartCmdText(at_parse_get_text(scanner), at_parse_get_leng(scanner));
}

int atParseStartCmdNone(void *scanner)
{
    return atParseStartCmdText("AT", 2);
}

int atParseSetCmdType(void *scanner, uint8_t type)
{
    printf("AT CMD parse set cmd type=%d\n", type);
    return 0;
}

static int atParsePushParamText(uint8_t type, char *text, unsigned leng)
{
    printf("AT CMD parse push param type=%d len=%d: %s\n", type, leng, text);
    return 0;
}

int atParsePushRawText(void *scanner)
{
    return atParsePushParamText(AT_CMD_PARAM_TYPE_RAW, at_parse_get_text(scanner), at_parse_get_leng(scanner));
}

int atParsePushEmpty(void *scanner)
{
    return atParsePushParamText(AT_CMD_PARAM_TYPE_EMPTY, "", 0);
}

extern int at_parse_debug;
#define CMDLINE_MAX (16 * 1024)
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s cmdline\n", argv[0]);
        return -1;
    }

    char *cmdline = (char *)malloc(CMDLINE_MAX);
    sprintf(cmdline, "%s\r", argv[1]);

    unsigned length = strlen(cmdline);
    void *scanner = NULL;

    if (at_parse_lex_init_extra(NULL, &scanner) != 0)
    {
        printf("at_parse_lex_init_extra failed\n");
        goto free_exit;
    }

    if (at_parse__scan_bytes(cmdline, length, scanner) == NULL)
    {
        printf("at_parse__scan_bytes failed\n");
        goto destroy_exit;
    }

#ifdef PARSE_DEBUG
    at_parse_debug = 1;
    at_parse_set_debug(1, scanner);
#endif

    int result = at_parse_parse(scanner);
    if (result != 0)
    {
        printf("at_parse_parse failed: %d\n", result);
        goto destroy_exit;
    }

    printf("FINISHED\n");
    at_parse_lex_destroy(scanner);
    return 0;

destroy_exit:
    at_parse_lex_destroy(scanner);

free_exit:
    free(cmdline);
    return -1;
}
