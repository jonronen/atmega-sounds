#!/usr/bin/env python

import sys
import struct


WAV_HEADER_SIZE = 0x2c
WAV_DATALEN_OFFSET = 0x28


if len(sys.argv) < 3:
    print "Usage: %s WAV_FILE AMPLIFICATION_PRECENT" % sys.argv[0]
    sys.exit()

precentage = 100

try:
    precentage = int(sys.argv[2])
except ValueError:
    print "AMPLIFICATION_PRECENT must be a number (not %s)" % sys.argv[2]
    sys.exit()

f = open(sys.argv[1], "rb")
g = open(sys.argv[1][:-4]+"_1"+sys.argv[1][-4:], "wb")
g.write (f.read (WAV_HEADER_SIZE))

wav_data = f.read()
wav_data = list(wav_data)

for i in range(len(wav_data)):
    wav_sample = ord(wav_data[i])
    wav_sample = 0x80 + (wav_sample - 0x80) * precentage / 100
    wav_sample = min(255, wav_sample)
    wav_sample = max(0, wav_sample)
    wav_data[i] = chr(wav_sample)

g.write("".join(wav_data))
g.close()

f.close()

