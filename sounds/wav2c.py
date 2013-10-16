#!/usr/bin/env python

import sys

if len(sys.argv) < 2:
    print "Usage: %s WAV_FILE" % sys.argv[0]
    sys.exit()

f = open(sys.argv[1], "rb")
f.seek(0x2c)

g = open(sys.argv[1] + ".c", "wt")
g.write("#include <avr/pgmspace.h>\n")
g.write("\n")
g.write("#ifdef PROGMEM\n")
g.write("#undef PROGMEM\n")
g.write("#define PROGMEM __attribute__((section(\".progmem.data\")))\n")
g.write("#endif\n")
g.write("\n")
g.write("extern const PROGMEM unsigned char sound0[];\n")
g.write("\n")
g.write("const unsigned char sound0[] PROGMEM = {")


wav_data = f.read()

for i in range(len(wav_data)):
    if i % 16 == 0:
        g.write("\n ")
    if i % 4 == 0:
        g.write(" ")
    g.write("0x%02x," % ord(wav_data[i]))

g.write("\n};\n")
g.close()

f.close()

print "Converted %s to %s.c" % (sys.argv[1], sys.argv[1])

