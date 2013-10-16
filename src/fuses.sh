avrdude -v -v -v -v -patmega128 -cstk500v1 -P/dev/ttyACM0 -b19200 -U lfuse:w:0xe4:m -U hfuse:w:0xd7:m -U efuse:w:0xff:m

