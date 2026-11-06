"""Build the CoCo SDC stream test kit.

Creates in the output directory (default: ./out):
  STRM512.DAT   64 x 512 byte blocks with a known pattern (32768 bytes)
  STRMODD.DAT   the same 64 blocks plus 3 bytes of block 64 (32771 bytes)
  STRMTEST.DSK  35 track RS-DOS disk image holding STRMTEST.BIN

Block b of the data files:
  bytes 0-2   block number, 24 bit big-endian
  byte  3     complement of block number low byte
  bytes 4-511 (b + offset) & 0xFF

Usage: python mktest.py STRMTEST.BIN [outdir]
"""
import os
import sys

NBLK = 64
BLOCK = 512


def block_data(b):
    d = bytearray(BLOCK)
    d[0] = (b >> 16) & 0xFF
    d[1] = (b >> 8) & 0xFF
    d[2] = b & 0xFF
    d[3] = (~b) & 0xFF
    for i in range(4, BLOCK):
        d[i] = (b + i) & 0xFF
    return bytes(d)


# RS-DOS disk geometry
TRACKS, SECTORS, SECSIZE = 35, 18, 256
DIR_TRACK = 17
GRANULES = 68
GRAN_SECTORS = 9


def sec_offset(track, sector):  # sector is 1-based
    return (track * SECTORS + (sector - 1)) * SECSIZE


def gran_offset(g):
    track = g // 2
    if track >= DIR_TRACK:
        track += 1
    return sec_offset(track, (g % 2) * GRAN_SECTORS + 1)


def make_dsk(files):
    """files: list of (name, ext, ftype, ascii, data)"""
    img = bytearray(b"\xff" * (TRACKS * SECTORS * SECSIZE))
    fat = bytearray(b"\xff" * GRANULES)
    dirents = []
    next_gran = 0
    for name, ext, ftype, ascii_flag, data in files:
        nsec = max(1, (len(data) + SECSIZE - 1) // SECSIZE)
        ngran = (nsec + GRAN_SECTORS - 1) // GRAN_SECTORS
        grans = list(range(next_gran, next_gran + ngran))
        next_gran += ngran
        for i, g in enumerate(grans):
            chunk = data[i * GRAN_SECTORS * SECSIZE:(i + 1) * GRAN_SECTORS * SECSIZE]
            img[gran_offset(g):gran_offset(g) + len(chunk)] = chunk
            if i + 1 < len(grans):
                fat[g] = grans[i + 1]
            else:
                fat[g] = 0xC0 + (nsec - i * GRAN_SECTORS)
        last_bytes = len(data) - (nsec - 1) * SECSIZE
        ent = bytearray(32)
        ent[0:8] = name.ljust(8).encode("ascii")
        ent[8:11] = ext.ljust(3).encode("ascii")
        ent[11] = ftype
        ent[12] = 0xFF if ascii_flag else 0x00
        ent[13] = grans[0]
        ent[14] = (last_bytes >> 8) & 0xFF
        ent[15] = last_bytes & 0xFF
        dirents.append(bytes(ent))
    fat_sec = bytearray(SECSIZE)
    fat_sec[:GRANULES] = fat
    off = sec_offset(DIR_TRACK, 2)
    img[off:off + SECSIZE] = fat_sec
    off = sec_offset(DIR_TRACK, 3)
    for ent in dirents:
        img[off:off + 32] = ent
        off += 32
    return bytes(img)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    binfile = sys.argv[1]
    outdir = sys.argv[2] if len(sys.argv) > 2 else "out"
    os.makedirs(outdir, exist_ok=True)

    blocks = b"".join(block_data(b) for b in range(NBLK))
    with open(os.path.join(outdir, "STRM512.DAT"), "wb") as f:
        f.write(blocks)
    with open(os.path.join(outdir, "STRMODD.DAT"), "wb") as f:
        f.write(blocks + block_data(NBLK)[:3])

    with open(binfile, "rb") as f:
        bin_data = f.read()
    dsk = make_dsk([("STRMTEST", "BIN", 2, False, bin_data)])
    with open(os.path.join(outdir, "STRMTEST.DSK"), "wb") as f:
        f.write(dsk)

    for n in ("STRM512.DAT", "STRMODD.DAT", "STRMTEST.DSK"):
        print(f"{n:14} {os.path.getsize(os.path.join(outdir, n)):7} bytes")


if __name__ == "__main__":
    main()
