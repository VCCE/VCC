# dsk2flash.py -- rebuild the CoCo PSG FLASH image from PSGDUMP disks.
#
#   python dsk2flash.py OUT.ROM DISK1.DSK [DISK2.DSK ...]
#
# Reads every PSGnn.BIN written by PSGDUMP.BAS (segment nn, loaded at
# $6000-$7FFF) and places it at nn*8K. Segments not found were blank and
# are left as $FF.
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from decbdsk import Disk, ml_segments

SEG = 0x2000


def main(argv):
    if len(argv) < 3:
        print(__doc__ if __doc__ else "usage: dsk2flash.py OUT.ROM DISK.DSK...")
        return 2
    img = bytearray(b"\xFF" * (64 * SEG))
    found = {}
    for path in argv[2:]:
        disk = Disk.load(path)
        for name, ext, e in disk.entries():
            if not (name.startswith("PSG") and len(name) == 5 and name[3:].isdigit()):
                continue
            seg = int(name[3:])
            segs, _ = ml_segments(disk.read(e))
            data = bytearray(b"\xFF" * SEG)
            for addr, chunk in segs:
                lo = addr - 0x6000
                if lo < 0 or lo + len(chunk) > SEG:
                    raise ValueError("%s: block $%04X outside $6000-$7FFF" % (name, addr))
                data[lo:lo + len(chunk)] = chunk
            if seg in found:
                print("warning: segment %d on %s and %s; using the latter" % (seg, found[seg], path))
            img[seg * SEG:(seg + 1) * SEG] = data
            found[seg] = path
    open(argv[1], "wb").write(img)
    print("wrote %s: segments %s, others blank" % (argv[1], sorted(found)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
