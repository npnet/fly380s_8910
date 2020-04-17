#!/usr/bin/env python3
# Copyright (C) 2019 RDA Technologies Limited and/or its affiliates("RDA").
# All rights reserved.
#
# This software is supplied "AS IS" without any warranties.
# RDA assumes no responsibility or liability for the use of the software,
# conveys no license or title under any patent, copyright, or mask work
# right to the product. RDA reserves the right to make changes in the
# software without notification.  RDA also make no representation or
# warranty that such application will be suitable for the specified use
# without further testing or modification.

import os
import sys
import struct
import json
import argparse
import re

NULL_STR = "".encode()
ARCHIVE_ALIGN_SIZE = 512
DESCRIPTION = """
Tool to create CPIO archive(only OLD BINARY LE support)
"""

class CpioOldLeFile():
    """
    a file in CPIO archive
    """

    CPIO_FILE_FORMAT = '=2s7HIHI{}s{}x{}s{}'
    CPIO_OLDLE_MAGIC = b'\xc7\x71'

    def __init__(self, abspath, escape, origin, to):
        cpio_path_ = abspath.replace(escape, "").replace("\\", "/").replace(origin, to)
        self.cpio_path = cpio_path_[re.search(r'[^/]', cpio_path_).start():].encode()
        self.magic = self.CPIO_OLDLE_MAGIC

        try:
            fstat = os.stat(abspath)
            self.device_num = self._us(fstat.st_dev)
            self.inode_num = self._us(fstat.st_ino)
            self.mode = fstat.st_mode
            self.user_id = self._us(fstat.st_uid)
            self.group_id = self._us(fstat.st_gid)
            self.num_link = self._us(fstat.st_nlink)
            self.rdevice_num = 0
            self.mtime = self._ul_to_ml(int(fstat.st_mtime))
            self.fname_size = len(self.cpio_path) + 1
            self.file_size = 0 if os.path.isdir(abspath) else fstat.st_size
            if self.file_size != 0:
                with open (abspath, 'rb') as f:
                    self.file_data = f.read()
            else:
                self.file_data = ''.encode()

        except Exception as e:
            raise Exception("{}".format(e))

        except IOError:
            raise Exception("No such file {}".format(abspath))

        self.fmt = self.CPIO_FILE_FORMAT.format(self.fname_size - 1, 1 if self.fname_size % 2 == 0 else 2,
                                                self.file_size, 'x' if self.file_size % 2 == 1 else '')
        self.file_size = self._ul_to_ml(self.file_size)

    def _us(self, num):
        return (num & 0xffff)

    def _ul_to_ml(self, num):
        return ((num & 0xffff) << 16) | ((num & 0xffff0000) >> 16)

    def data(self):
        return struct.pack(self.fmt, self.magic, self.device_num,
                           self.inode_num, self.mode, self.user_id, self.group_id,
                           self.num_link, self.rdevice_num, self.mtime, self.fname_size,
                           self.file_size, self.cpio_path, self.file_data)


class CpioOldLeEosFile(CpioOldLeFile):

    def __init__(self):
        self.magic = self.CPIO_OLDLE_MAGIC
        self.device_num = 0
        self.inode_num = 0
        self.mode = 0
        self.user_id = 0
        self.group_id = 0
        self.num_link = 1
        self.rdevice_num = 0
        self.mtime = 0
        self.cpio_path = b"TRAILER!!!"
        self.fname_size = 11
        self.file_size = 0
        self.file_data = ''.encode()
        self.fmt = "=2s7HIHI10sx0s"


class CpioOldLE():
    """
    CPIO archive generator
    """

    def __init__(self):
        self._eos = False
        self.cpio_files = []

    def set_eos(self):
        self.cpio_files.append(CpioOldLeEosFile())
        self._eos = True

    def _insert_file(self, abspath, base, origin, to):
        self.cpio_files.append(CpioOldLeFile(abspath, base, origin, to))
        if os.path.isdir(abspath):
            for f in os.listdir(abspath):
                self._insert_file(os.path.join(abspath, f), base, origin, to)

    def add_path(self, base, origin, to):
        if self._eos:
            return False
        absori = os.path.join(base, origin)
        origin = origin.replace('\\', '/')
        self._insert_file(absori, base, origin, to)
        return True

    def dump_to_file(self, output_abs):
        if not self._eos:
            return False
        data = b""
        for c in self.cpio_files:
            data += c.data()
        data_len = len(data)
        fmt = "{}s{}x".format(data_len, ARCHIVE_ALIGN_SIZE - data_len % ARCHIVE_ALIGN_SIZE)
        with open(output_abs, 'wb') as f:
            f.write(struct.pack(fmt, data))

def cpiogen(args):
    try:
        config_abs = os.path.abspath(args.config)
        out_abs = os.path.abspath(args.output)
        with open (config_abs, 'r') as f:
            cfg = json.load(f)
        if cfg['format'] != 'OldBinaryLE':
            raise Exception("Only support OldBinaryLE format")

        cpio = CpioOldLE()
        for f in cfg['files']:
            if f['origin'].encode() != NULL_STR:
                if 'to' not in f.keys() or f['to'] == "":
                    f['to'] = f['origin']
                cpio.add_path(cfg['base_path'], f['origin'], f['to'])
        cpio.set_eos()
        cpio.dump_to_file(out_abs)
        return 0

    except Exception as err:
        print(err)
        return -1

def get_all_subfiles(abspath, lst):
    lst.append(abspath)
    if os.path.isdir(abspath):
        for f in os.listdir(abspath):
            get_all_subfiles(os.path.join(abspath, f), lst)

def cmakegen(args):
    try:
        config_abs = os.path.abspath(args.config)
        cmake_abs = os.path.abspath(args.cmake)
        with open(config_abs, 'r') as f:
            cfg = json.load(f)
        depends = []
        for f in cfg['files']:
            if f['origin'].encode() != NULL_STR:
                abs_path = os.path.join(cfg['base_path'], f['origin']).replace("\\", "/")
                get_all_subfiles(abs_path, depends)

        with open(cmake_abs, 'w') as f:
            f.write("# Auto Generated\n")
            if len(depends) != 0:
                f.write("set(package_file_depends\n")
                for d in depends:
                    f.write("    {}\n".format(d))
                f.write(")\n")

    except Exception as err:
        print(err)
        return -1

def main(argv):
    parser = argparse.ArgumentParser(description=DESCRIPTION)
    subparser = parser.add_subparsers(description="")

    cpiogen_parser = subparser.add_parser("archive", help="generate cpio archive file")
    cpiogen_parser.set_defaults(func=cpiogen)
    cpiogen_parser.add_argument('--config', dest='config', help='config file')
    cpiogen_parser.add_argument('--output', dest='output', help='output cpio archive')

    cmakegen_parser = subparser.add_parser('cmake', help="generate cmake dependancy")
    cmakegen_parser.set_defaults(func=cmakegen)
    cmakegen_parser.add_argument('--config', dest='config', help='config file')
    cmakegen_parser.add_argument('--cmake', dest='cmake', help='output .cmake file')

    args = parser.parse_args(argv)
    if args.__contains__('func'):
        return args.func(args)
    parser.parse_args(['-h'])
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))