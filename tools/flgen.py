#!/bin/env python

import argparse
import os
import struct

parser = argparse.ArgumentParser(description="Generate a MIPS .fl file")
parser.add_argument("input", metavar="input.bin", help="input binary")
parser.add_argument("output", metavar="output.fl", help="output .fl file")
args = parser.parse_args()

base = 0x1fc00000

with open(args.output, "w+") as outfile:
    outfile.write("!R\n")
    outfile.write(">%05xxxx @%08x !C\n" % (base >> 12, base))

    with open(args.input, "rb") as infile:
        infile.seek(0, os.SEEK_END)
        insize = infile.tell()
        end = base + insize - 1
        infile.seek(0, os.SEEK_SET)

        sect = base
        while sect < end:
            outfile.write(">%05xxxx @%08x !E\n" % (sect >> 12, sect))
            sect = sect + (128 * 1024)

        outfile.write("@%08x\n" % base)
        outfile.write(">%08x\n" % base)

        addr = base
        for chunk in iter(lambda: infile.read(4), ''):
            if len(chunk) == 0:
                break

            while len(chunk) < 4:
                chunk = list(b for b in chunk)
                chunk.append(0xff)
                chunk = bytes(chunk)

            if addr > base and (addr % (128 * 1024) == 0):
                outfile.write("@%08x\n" % addr)
                outfile.write(">%08x\n" % addr)

            val = struct.unpack('<I', chunk)[0]
            outfile.write("%08x\n" % val)
            addr = addr + 4

        while addr % 64 != 0:
            outfile.write("ffffffff\n")
            addr = addr + 4

        outfile.write(">LOCKFLSH\n")

        sect = base
        while sect < end:
            outfile.write("@%08x !S\n" % sect)
            sect = sect + (128 * 1024)

    outfile.write(">#DL_DONE\n")
    outfile.write(">FINISHED\n")
