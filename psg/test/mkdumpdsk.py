# mkdumpdsk.py -- builds out/PSGDUMP.DSK holding PSGDUMP.BAS (ASCII) and
# a blank out/PSGDMP2.DSK for segments that don't fit on the first disk.
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
from decbdsk import Disk
bas = open(os.path.join(here, "psgdump.bas"), "rb").read().replace(b"\r\n", b"\r")
d = Disk()
d.add("PSGDUMP", "BAS", 0, 0xFF, bas)
d.save(os.path.join(here, "out", "PSGDUMP.DSK"))
Disk().save(os.path.join(here, "out", "PSGDMP2.DSK"))
print("wrote out/PSGDUMP.DSK (%d byte program) and out/PSGDMP2.DSK" % len(bas))
