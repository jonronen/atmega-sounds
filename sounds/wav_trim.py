#!/usr/bin/env python

import sys
import struct


WAV_HEADER_SIZE = 0x2c
WAV_DATALEN_OFFSET = 0x28


if len(sys.argv) < 2:
    print "Usage: %s WAV_FILE [START_OFFSET [END_OFFSET]]" % sys.argv[0]
    sys.exit()

start_ind = WAV_HEADER_SIZE
end_ind = 0xffffff

if len(sys.argv) >= 3:
    try:
        if sys.argv[2].lower().startswith('0x'):
            start_ind = int(sys.argv[2][2:], 16)
        else:
            start_ind = int(sys.argv[2])
    except ValueError:
        print "START_OFFSET must be a number (not %s)" % sys.argv[2]
        sys.exit()
    if start_ind < WAV_HEADER_SIZE:
        start_ind = WAV_HEADER_SIZE

if len(sys.argv) >= 4:
    try:
        if sys.argv[3].lower().startswith('0x'):
            end_ind = int(sys.argv[3][2:], 16)
        else:
            end_ind = int(sys.argv[3])
    except ValueError:
        print "END_OFFSET must be a number (not %s)" % sys.argv[2]
        sys.exit()
    if end_ind < start_ind:
        end_ind = start_ind

f = open(sys.argv[1], "rb")
g = open(sys.argv[1][:-4]+"_1"+sys.argv[1][-4:], "wb")
g.write (f.read (WAV_HEADER_SIZE))
f.seek(start_ind)

wav_data = f.read(end_ind - start_ind)

g.write(wav_data)
g.seek(WAV_DATALEN_OFFSET)
g.write(struct.pack("<I", len(wav_data)))
g.close()

f.close()

