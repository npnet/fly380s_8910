#!/usr/bin/env python3
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

import argparse
import sys
import csv
import re

DESCRIPTION = """
Generate iomux source files from csv
"""


def sort_nice(l):
    def convert(text): return int(text) if text.isdigit() else text

    def alphanum_key(key): return [convert(c)
                                   for c in re.split('([0-9]+)', key)]
    l.sort(key=alphanum_key)


def main():
    parser = argparse.ArgumentParser(description=DESCRIPTION)

    parser.add_argument("csv",
                        help="csv file for pin mux")
    parser.add_argument("pindef", nargs='?', default='hal_iomux_pindef.h',
                        help="output iomux pin define file")
    parser.add_argument("pincfg", nargs='?', default='hal_iomux_pincfg.h',
                        help="output iomux pin config file")

    args = parser.parse_args()

    cfg = {}
    pins = []
    with open(args.csv, 'r') as fh:
        pinmux = csv.reader(fh)
        for row in pinmux:
            if not row[0]:
                continue
            reg = row[0]
            n = -1
            for pin in row[1:]:
                n += 1
                if not pin:
                    continue
                if pin.startswith('x--'):
                    continue
                if pin in pins:
                    raise Exception('duplicated pin: {}'.format(pin))
                pins.append(pin)
                cfg[pin] = (reg, n)

    sort_nice(pins)

    with open(args.pindef, 'w') as fh:
        fh.write("#ifndef _IOMUX_PINDEF_H_\n")
        fh.write("#define _IOMUX_PINDEF_H_\n\n")
        fh.write('// Auto generated. Don\'t edit it manually!\n\n')
        fh.write('enum {\n')
        for pin in pins:
            fh.write('    PINFUNC_{},\n'.format(pin.upper()))
        fh.write('};\n')
        fh.write("\n#endif\n\n")

    with open(args.pincfg, 'w') as fh:
        fh.write("#ifndef _IOMUX_PINCFG_H_\n")
        fh.write("#define _IOMUX_PINCFG_H_\n\n")
        fh.write('// Auto generated. Don\'t edit it manually!\n\n')
        fh.write('static const halIomuxConfig_t gHalIomuxConfig[] = {\n')
        for pin in pins:
            reg, n = cfg[pin]
            if reg == 'NULL':
                fh.write(
                    '    {{{}, {}}}, // PINFUNC_{}\n'.format(reg, n, pin.upper()))
            else:
                fh.write(
                    '    {{&{}, {}}}, // PINFUNC_{}\n'.format(reg, n, pin.upper()))
        fh.write('};\n')
        fh.write("\n#endif\n\n")


if __name__ == "__main__":
    main()
