# Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
# All rights reserved.
#
# This software is supplied "AS IS" without any warranties.
# RDA assumes no responsibility or liability for the use of the software,
# conveys no license or title under any patent, copyright, or mask work
# right to the product. RDA reserves the right to make changes in the
# software without notification.  RDA also make no representation or
# warranty that such application will be suitable for the specified use
# without further testing or modification.

ifeq "$(DEBUG)" "1"
FLEX := flex -d
BISON := bison --debug
CFLAGS := -DPARSE_DEBUG
else
FLEX := flex
BISON := bison
CFLAGS += -DYY_MEM_ONLY -DYY_NO_ERRNO
endif

CFLAGS += -I../../kernel/include -I../include -I../src

all:
	$(FLEX) --never-interactive at_parse.lex
	$(BISON) at_parse.y
	gcc -g -O0 $(CFLAGS) -I$(PROJECT_OUT)/include -I. -o at_parse_test at_parse.y.c at_parse.lex.c at_parse_test.c

clean:
	rm -f at_*.lex.* *.y.*  at_parse_test
