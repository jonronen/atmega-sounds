#!/usr/bin/env python

import sys


WAV_HEADER_SIZE = 0x2c


if len(sys.argv) < 3:
    print "Usage: %s WAV_FILE INDEX" % sys.argv[0]
    sys.exit()

try:
    ind = int(sys.argv[2])
except ValueError:
    print "INDEX must be a number (not %s)" % sys.argv[2]
    sys.exit()

f = open(sys.argv[1], "rb")
f.seek(WAV_HEADER_SIZE)

g = open(sys.argv[1] + ".c", "wt")
g.write("#include <avr/pgmspace.h>\n")
g.write("\n")
g.write("#ifdef PROGMEM\n")
g.write("#undef PROGMEM\n")
g.write("#define PROGMEM __attribute__((section(\".progmem.data\")))\n")
g.write("#endif\n")
g.write("\n")
g.write("extern const PROGMEM unsigned char sound%d[];\n" % ind)
g.write("extern const uint16_t sound%d_len;" % ind)
g.write("\n")
g.write("const unsigned char sound%d[] PROGMEM = {" % ind)
g.write("\n  0x00,0x04,0x08,0x0c, 0x10,0x14,0x18,0x1c, 0x20,0x24,0x28,0x2c, 0x30,0x34,0x38,0x3c,")
g.write("\n  0x40,0x44,0x48,0x4c, 0x50,0x54,0x58,0x5c, 0x60,0x64,0x68,0x6c, 0x70,0x74,0x78,0x7c,")


wav_data = f.read()

for i in range(len(wav_data)):
    if i % 16 == 0:
        g.write("\n ")
    if i % 4 == 0:
        g.write(" ")
    g.write("0x%02x," % ord(wav_data[i]))

g.write("\n  0x7c,0x78,0x74,0x70, 0x6c,0x68,0x64,0x60, 0x5c,0x58,0x54,0x50, 0x4c,0x48,0x44,0x40,")
g.write("\n  0x3c,0x38,0x34,0x30, 0x2c,0x28,0x24,0x20, 0x1c,0x18,0x14,0x10, 0x0c,0x08,0x04,0x00,")
g.write("\n};\n")

g.write("const uint16_t sound%d_len = sizeof(sound%d);\n" % (ind,ind))

g.close()

f.close()

print "Converted %s to %s.c" % (sys.argv[1], sys.argv[1])

