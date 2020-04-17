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
import json

DESCRIPTION = """
Create modem image use fbdevgen

Configuration json format:
* modemnvm_dir: modem fixed nv directory in file system
* lzma_block_size: blocked LZMA block size
* fbdev_name: flash block device name, to search in partition information
* sffs_name: SFFS file system name, to search in partition information
* modemnv (array)
  * nvid: cpnv id
  * file: nv file name, not including directory
* modembin (array)
  * file: file name in file system
  * local_file: (optional) file name in PC
  * lzma3: (default False)

Also,
* It will copy modem bin to <gendir>/modem, which is for debug only.
* Hack memory_index_list.bin for LZMA flags.
"""

FLAG_LZMA = 0x100
FLAG_LZMA2 = 0x200
FLAG_LZMA3 = 0x400
FLAG_LZMA_MASK = 0x700


def find_nv_fname(cfg, nvid):
    for nv in cfg['modemnv']:
        mid = int(nv['nvid'], 0)
        if mid == nvid:
            return nv['file']
    return None


def find_fbdev(cfg, name):
    for d in cfg['descriptions']:
        if d['name'] == name:
            return d
    return None


def split_nv(nvbin, cfg, nvdir):
    nvbins = []

    fh = open(nvbin, 'rb')
    nvdata = fh.read()
    fh.close()

    pos = 4
    size = len(nvdata)
    while pos < size:
        nvid, nvsize = struct.unpack('HH', nvdata[pos:pos+4])
        if nvid == 0xffff:
            break

        fname = find_nv_fname(cfg, nvid)
        if not fname:
            pass  # print('ignore nvid 0x%x' % nvid)
        else:
            nvbin = os.path.join(nvdir, fname)
            nvbins.append(nvbin)

            fh = open(nvbin, 'wb')
            fh.write(nvdata[pos+4:pos+4+nvsize])
            fh.close()

        nvsize = (nvsize + 3) & 0xfffffffc
        pos += 4 + nvsize
    return nvbins


def gen_add_file(cfg, fname, infname):
    if 'plain_file' not in cfg:
        cfg['plain_file'] = []
    cfg['plain_file'].append({'file': infname, 'local_file': fname})


def gen_add_memlist(cfg, fname):
    if 'memlist' not in cfg:
        cfg['memlist'] = {}
    cfg['memlist']['file'] = 'mem_index_list'
    cfg['memlist']['local_file'] = fname


def gen_add_lzma3(cfg, fname, infname):
    if 'lzma3_file' not in cfg:
        cfg['lzma3_file'] = []
    cfg['lzma3_file'].append({'file': infname, 'local_file': fname})


def hack_mem_list(infname, outfname, cfg):
    fh = open(infname, 'rb')
    indata = fh.read()
    fh.close()

    pos = 0
    insize = len(indata)
    outdata = bytearray()
    while pos < insize:
        name, address, size, flags = struct.unpack(
            '20sIII', indata[pos:pos+32])
        pos += 32

        flags &= ~FLAG_LZMA_MASK
        rname = name.rstrip(b'\x00').decode('utf-8')
        for mbin in cfg['modembin']:
            if mbin['file'] == rname:
                if 'lzma' in mbin:
                    flags |= FLAG_LZMA
                elif 'lzma2' in mbin:
                    flags |= FLAG_LZMA2
                elif 'lzma3' in mbin:
                    flags |= FLAG_LZMA3

        outdata += struct.pack(
            '20sIII', name, address, size, flags)

    fh = open(outfname, 'wb')
    fh.write(outdata)
    fh.close()


def main():
    parser = argparse.ArgumentParser(description=DESCRIPTION,
                                     formatter_class=argparse.RawTextHelpFormatter)

    parser.add_argument('--config', dest='config', required=True,
                        help='The json file for modem image')
    parser.add_argument('--partinfo', dest='partinfo', required=True,
                        help='The json file for partition information')
    parser.add_argument('--bindir', dest='bindir', required=True,
                        help='Modem bin directory')
    parser.add_argument('--gendir', dest='gendir', required=True,
                        help='Modem generator')
    parser.add_argument('--nvbin', dest='nvbin', required=True,
                        help='Input nvbin file name')
    parser.add_argument('--img', dest='img', required=True,
                        help='Output modem.img')

    args = parser.parse_args()

    gencfg = {}

    modemgendir = args.gendir
    modemgenscriptdir = args.gendir
    modemjsongen = os.path.join(modemgenscriptdir, 'modem.json')
    memlistgen = os.path.join(modemgenscriptdir, 'memory_index_list.bin')

    # Copy modem bin, just to make modem bin seeable in hex
    if os.path.exists(modemgendir):
        shutil.rmtree(modemgendir)
    os.makedirs(modemgendir)
    for file in os.listdir(args.bindir):
        if os.path.isfile(os.path.join(args.bindir, file)):
            srcfile = os.path.join(args.bindir, file)
            targetfile = os.path.join(modemgendir, file)
            shutil.copy(srcfile, targetfile)

    if not os.path.exists(modemgenscriptdir):
        os.makedirs(modemgenscriptdir)

    # Load configuration json file
    fh = open(args.config, 'r')
    cfg = json.load(fh)
    fh.close()

    # Load partition configuration
    fh = open(args.partinfo, 'r')
    parti = json.load(fh)
    fh.close()

    # Hack memory_index_list.bin
    hack_mem_list(os.path.join(args.bindir, "memory_index_list.bin"),
                  memlistgen, cfg)

    nvdir_in_img = cfg['modemnv_dir']

    # Split modem nv
    nvdir = os.path.join(args.gendir, nvdir_in_img)
    if not os.path.exists(nvdir):
        os.makedirs(nvdir)

    split_nvbins = split_nv(args.nvbin, cfg, nvdir)
    for nv in split_nvbins:
        nv_fname = os.path.basename(nv)
        gen_add_file(gencfg, nv, os.path.join(nvdir_in_img, nv_fname))

    # Add modem bin
    for cpbin in cfg['modembin']:
        fname = cpbin['file']
        local_fname = cpbin.get('local_file', fname)

        if local_fname == 'memory_index_list.bin':
            local_fname = memlistgen
        else:
            local_fname = os.path.join(args.bindir, local_fname)

        if os.path.exists(local_fname):
            if cpbin.get('lzma3', False):
                gen_add_lzma3(gencfg, local_fname, fname)
            else:
                gen_add_file(gencfg, local_fname, fname)

    # Add lzma option
    gencfg['lzma_block_size'] = cfg['lzma_block_size']

    # Partition options
    gencfg['offset'] = '0'
    gencfg['count'] = '0'

    # Get flash block device options
    fbdev = find_fbdev(parti, cfg['fbdev_name'])
    mgen = {}
    mgen['type'] = fbdev['type']
    mgen['offset'] = fbdev['offset']
    mgen['size'] = fbdev['size']
    mgen['erase_block'] = fbdev['erase_block']
    mgen['logic_block'] = fbdev['logic_block']
    mgen['partiton'] = []
    mgen['partiton'].append(gencfg)

    # Write configuration to json
    fh = open(modemjsongen, 'w')
    json.dump(mgen, fh)
    fh.close()

    # Run flash block device generator
    subprocess.run(['dtools', 'fbdevgen', modemjsongen, args.img])


if __name__ == "__main__":
    main()
