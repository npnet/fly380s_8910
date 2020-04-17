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
import struct
import shutil
import os
import glob
import subprocess
import platform
import re

DESCRIPTION = """
List nvid or extract nvid from nvitem.bin
"""


def nvbin_list_args(sub_parser):
    parser = sub_parser.add_parser('list', help='list all nvid')
    parser.set_defaults(func=nvbin_list)
    parser.add_argument('nvbin', help='nvitem.bin')


def nvbin_list(args):
    fh = open(args.nvbin, 'rb')
    nvdata = fh.read()
    fh.close()

    pos = 4
    size = len(nvdata)
    while pos < size:
        nvid, nvsize = struct.unpack('HH', nvdata[pos:pos+4])
        if nvid == 0xffff:
            break

        print('nvid 0x%x: %d' % (nvid, nvsize))
        nvsize = (nvsize + 3) & 0xfffffffc
        pos += 4 + nvsize
    return 0


def nvbin_extract_args(sub_parser):
    parser = sub_parser.add_parser('extract', help='extract nvid')
    parser.set_defaults(func=nvbin_extract)
    parser.add_argument('nvbin', help='nvitem.bin')
    parser.add_argument('nvid', help='nvid to be extracted')
    parser.add_argument('output', help='output file name')


def nvbin_extract(args):
    fh = open(args.nvbin, 'rb')
    nvdata = fh.read()
    fh.close()

    pos = 4
    size = len(nvdata)
    while pos < size:
        nvid, nvsize = struct.unpack('HH', nvdata[pos:pos+4])
        if nvid == 0xffff:
            break

        if nvid == int(args.nvid, 0):
            print('extract nvid 0x%x: %d' % (nvid, nvsize))
            fh = open(args.output, 'wb')
            fh.write(nvdata[pos+4:pos+4+nvsize])
            fh.close()
            return 0

        nvsize = (nvsize + 3) & 0xfffffffc
        pos += 4 + nvsize

    print('nvid 0x%x not found' % nvid)
    return -1


NVBIN_GEN_DESCRIPTION = '''
Generate nvitem.bin from prj and nvm files. It will use NVGen for
generation. However, NVGen may change prj and nvm files. So, it will
copy the input to a generation directory, and run NVGen.
'''


def nvbin_gen_args(sub_parser):
    parser = sub_parser.add_parser('gen', help='generate from prj',
                                   description=NVBIN_GEN_DESCRIPTION,
                                   formatter_class=argparse.RawTextHelpFormatter)
    parser.set_defaults(func=nvbin_gen)
    parser.add_argument('--gendir', dest='gendir', default=None,
                        help='generation directory')
    parser.add_argument('--fix-size', dest='fixsize', default=None,
                        help='padding to fixed size')
    parser.add_argument('prj', help='prj for nvitems')
    parser.add_argument('output', help='output nvitem.bin')


def get_target_from_prj(prj):
    r = re.compile(r'TARGET\s*=\s*(\S+)')

    fh = open(prj, 'r')
    for line in fh.readlines():
        m = r.search(line)
        if m is not None:
            return m.group(1)
    raise Exception('TARGET not found in ', prj)


def get_modules_from_prj(prj):
    r = re.compile(r'MODULE\s*=\s*(\S+)')

    modules = []
    fh = open(prj, 'r')
    for line in fh.readlines():
        m = r.search(line)
        if m is not None:
            modules.append(m.group(1))

    if not modules:
        raise Exception('MODULE not found in ', prj)
    return modules


def nvbin_gen(args):
    target = get_target_from_prj(args.prj)
    modules = get_modules_from_prj(args.prj)

    if not args.gendir:
        gendir = os.path.dirname(args.prj)
        prj_dest = args.prj
    else:
        if os.path.exists(args.gendir):
            shutil.rmtree(args.gendir)
        os.makedirs(args.gendir)

        gendir = args.gendir
        prj_dest = os.path.join(args.gendir, os.path.basename(args.prj))
        prj_dir = os.path.dirname(args.prj)
        shutil.copyfile(args.prj, prj_dest)
        for module in modules:
            shutil.copyfile(os.path.join(prj_dir, module),
                            os.path.join(args.gendir, module))

    subprocess.run(['NVGen', prj_dest, '-L'])

    outdir = os.path.dirname(args.output)
    if outdir and not os.path.exists(outdir):
        os.makedirs(outdir)

    fh = open(os.path.join(gendir, target), 'rb')
    bindata = fh.read()
    fh.close()

    if args.fixsize:
        fixsize = int(args.fixsize, 0)
        if len(bindata) > fixsize:
            raise Exception('nvbin exceed fixed size')

        bindata += b'\xff' * (fixsize - len(bindata))

    fh = open(args.output, 'wb')
    fh.write(bindata)
    fh.close()


NVBIN_DIFFS_DESCRIPTION = '''
Generate delta_nv.bin from prj and parameter delta nv files.
However, cpp_only may change nv files and delta_nv.bin will generate in input path. So, it will
copy the input to a generation directory, and run NVGen.
'''


def nvbin_diffs_args(sub_parser):
    parser = sub_parser.add_parser('diffs', help='generate from param delta nv',
                                   description=NVBIN_DIFFS_DESCRIPTION,
                                   formatter_class=argparse.RawTextHelpFormatter)
    parser.set_defaults(func=nvbin_diffs)
    parser.add_argument('--src', dest='src', default=None,
                        help='input param delat nv file')
    parser.add_argument('--des', dest='des', default=None,
                        help='generation param delta nv file')
    parser.add_argument('prj', help='prj for nvitems')


def nvbin_diffs(args):
    if not args.src:
        return -1

    if not args.des:
        desdir = os.path.dirname(args.src)
    else:
        desdir = os.path.dirname(args.des)
        if os.path.exists(desdir):
            shutil.rmtree(desdir)

        os.makedirs(desdir)
        shutil.copyfile(args.src, args.des)

    # Write empty Index.xml, to avoid annoying message from NVGen
    with open(os.path.join(desdir, 'Index.xml'), 'w') as fh:
        fh.write('<Operators version="1"/>\n')

    prj_os = args.prj.replace('\\', '/')
    desdir_os = desdir.replace('\\', '/')
    if platform.system() == 'Windows':
        prj_os = args.prj.replace('/', '\\')
        desdir_os = desdir.replace('/', '\\')
    subprocess.run(['NVGen', prj_os, '-L', '-c', desdir_os])


NVBIN_APPLY_DESCRIPTION = '''
Apply delta_nv.bin to nvitem.bin.
'''


def nvbin_apply_args(sub_parser):
    parser = sub_parser.add_parser('apply', help='apply delta_nv.bin to nvitem.bin',
                                   description=NVBIN_APPLY_DESCRIPTION,
                                   formatter_class=argparse.RawTextHelpFormatter)
    parser.set_defaults(func=nvbin_apply)
    parser.add_argument('--verbose', dest='verbose', action='store_true',
                        help='verbose information output')
    parser.add_argument('--nvbin', dest='nvbin', default=None,
                        help='nvitem.bin')
    parser.add_argument('--deltanv', dest='deltanv', nargs='+', default=None,
                        help='delta_nv.bin')
    parser.add_argument('--output', dest='output', default=None,
                        help='output nvitem.bin')


def nvdata_modify(nvdata, nvid, offset, dlen, ddata):
    pos = 4
    size = len(nvdata)
    while pos < size:
        bin_nvid, nvsize = struct.unpack('HH', nvdata[pos:pos+4])
        if bin_nvid == 0xffff:
            break
        if bin_nvid == nvid:
            nvdata[pos+4+offset:pos+4+offset+dlen] = ddata
            return
        pos += 4 + nvsize
    raise Exception('nvid {} not found')


def nvbin_apply(args):
    if not args.nvbin or not args.deltanv:
        return -1
    fh = open(args.nvbin, 'rb')
    nvdata = bytearray(fh.read())
    fh.close()
    for deltanv in args.deltanv:
        fh = open(deltanv, 'rb')
        delta = fh.read()
        fh.close()
        size = len(delta)
        pos = 0
        while pos < size:
            _, dsize = struct.unpack('<HI', delta[pos+20:pos+26])
            dpos = pos + 26
            while dpos < pos + dsize - 2:
                nvid, offset, dlen = struct.unpack('HHH', delta[dpos:dpos+6])
                if args.verbose:
                    print('deltanv id/{} offset/{} len/{}'.format(nvid, offset, dlen))
                ddata = delta[dpos+6:dpos+6+dlen]
                nvdata_modify(nvdata, nvid, offset, dlen, ddata)
                dpos += 6 + dlen
            pos = dpos + 2
    fh = open(args.output, 'wb')
    fh.write(nvdata)
    fh.close()


def main(argv):
    parser = argparse.ArgumentParser(description=DESCRIPTION,
                                     formatter_class=argparse.RawTextHelpFormatter)
    sub_parser = parser.add_subparsers(help='sub-commands')

    nvbin_list_args(sub_parser)
    nvbin_extract_args(sub_parser)
    nvbin_gen_args(sub_parser)
    nvbin_diffs_args(sub_parser)
    nvbin_apply_args(sub_parser)

    args = parser.parse_args(argv)
    if args.__contains__('func'):
        return args.func(args)

    parser.parse_args(['-h'])
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
