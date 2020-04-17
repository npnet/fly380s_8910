# README

Usually, this directory won't be release as source codes. Rather, a prebuilt
object file `atr/core/<SOC>/atr_core2.o` will be provided. To build with
source codes, it is needed to replace `core.cmake` under this directory to
parent directory. Also, it is suggested to delete `atr/core/<SOC>/atr_core2.o`
to avoid confusion.

## Modify at_parse.lex or at_parse.y

By default, the pre-generated files `at_parse.lex.c` is used, which is
generated from `at_parse.lex` by `flex`. The verified `flex` is
`prebuilts/linux/bin/flex`, and only verified on Ubuntu 16.04. The system
version of `flex` can't work.

By default, the pre-generated files `at_parse.y.c` is used, which is
generated from `at_parse.y` by `bison`. The verified `bison` is system
version of Ubuntu 16.04 (3.0.4).

To run `flex` and `bison`, just run `make` in this directory.

## at_parse_test

There is a test program in this directory. `make` will build `at_parse_test`.
This will run parser only for AT command line. Examples:

    $ ./at_parse_test "ATE0V1"

The parameter is a complete AT command line. The leading `AT` is needed,
and the `\r` at the end of AT command line must **not** present.
