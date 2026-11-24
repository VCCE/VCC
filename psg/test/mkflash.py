# mkflash.py -- builds out/psgtest.rom (512K) for the CoCo PSG emulator.
# Segment 0 starts with autostart.bin; every other byte of segment N is N,
# so PEEKing a mapped bank shows which segment is mapped.
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
code = open(os.path.join(here, "out", "autostart.bin"), "rb").read()
img = bytearray(i // 0x2000 for i in range(0x80000))
img[0:len(code)] = code
open(os.path.join(here, "out", "psgtest.rom"), "wb").write(img)
print("wrote out/psgtest.rom,", len(code), "bytes of autostart code")
