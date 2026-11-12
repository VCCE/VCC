# decbdsk.py -- minimal Disk Extended Color BASIC (.DSK) reader/writer.
# 35 tracks x 18 sectors x 256 bytes; directory on track 17
# (sector 2 = FAT, sectors 3-11 = directory); 68 granules of 9 sectors.

SECT = 256
SPT = 18
TRACKS = 35
GRANS = 68
DIR_TRACK = 17


def _off(track, sector):  # sector is 1-based
    return (track * SPT + sector - 1) * SECT


def _gran_off(g):
    track = g // 2
    if track >= DIR_TRACK:
        track += 1
    return _off(track, (g % 2) * 9 + 1)


class Disk:
    def __init__(self, data=None):
        self.d = bytearray(data) if data else bytearray(b"\xFF" * (TRACKS * SPT * SECT))
        if data is None:
            fat = _off(DIR_TRACK, 2)
            self.d[fat:fat + SECT] = b"\xFF" * GRANS + b"\x00" * (SECT - GRANS)

    @classmethod
    def load(cls, path):
        return cls(open(path, "rb").read())

    def save(self, path):
        open(path, "wb").write(self.d)

    def _fat(self):
        o = _off(DIR_TRACK, 2)
        return self.d, o

    def entries(self):
        for s in range(3, 12):
            base = _off(DIR_TRACK, s)
            for i in range(8):
                e = base + i * 32
                if self.d[e] == 0xFF:
                    return
                if self.d[e] == 0x00:
                    continue
                name = bytes(self.d[e:e + 8]).decode("latin-1").rstrip()
                ext = bytes(self.d[e + 8:e + 11]).decode("latin-1").rstrip()
                yield name, ext, e

    def read(self, entry):
        d, fat = self._fat()
        e = entry
        g = d[e + 13]
        last = (d[e + 14] << 8) | d[e + 15]
        out = bytearray()
        for _ in range(GRANS):
            nxt = d[fat + g]
            o = _gran_off(g)
            if nxt >= 0xC0:
                n = nxt & 0x1F
                if n:
                    out += d[o:o + (n - 1) * SECT]
                    out += d[o + (n - 1) * SECT:o + (n - 1) * SECT + last]
                return bytes(out)
            out += d[o:o + 9 * SECT]
            g = nxt
        raise ValueError("FAT loop")

    def add(self, name, ext, ftype, ascii_flag, data):
        d, fat = self._fat()
        free = [g for g in range(GRANS) if d[fat + g] == 0xFF]
        need = max(1, -(-len(data) // (9 * SECT)))
        if need > len(free):
            raise IOError("disk full")
        # directory slot
        slot = None
        for s in range(3, 12):
            base = _off(DIR_TRACK, s)
            for i in range(8):
                e = base + i * 32
                if d[e] in (0x00, 0xFF):
                    slot = e
                    break
            if slot is not None:
                break
        if slot is None:
            raise IOError("directory full")
        grans = free[:need]
        lastbytes = 0
        for i, g in enumerate(grans):
            chunk = data[i * 9 * SECT:(i + 1) * 9 * SECT]
            o = _gran_off(g)
            d[o:o + len(chunk)] = chunk
            if i + 1 < need:
                d[fat + g] = grans[i + 1]
            else:
                nsec = max(1, -(-len(chunk) // SECT))
                d[fat + g] = 0xC0 | nsec
                lastbytes = len(chunk) - (nsec - 1) * SECT
        rec = name.ljust(8)[:8].encode() + ext.ljust(3)[:3].encode()
        rec += bytes([ftype, ascii_flag, grans[0], lastbytes >> 8, lastbytes & 0xFF])
        d[slot:slot + 32] = rec + b"\x00" * (32 - len(rec))


def ml_segments(blob):
    """Parse a SAVEM (machine language) file -> list of (addr, bytes), exec."""
    segs, i = [], 0
    while i < len(blob):
        kind = blob[i]
        if kind == 0x00:
            n = (blob[i + 1] << 8) | blob[i + 2]
            a = (blob[i + 3] << 8) | blob[i + 4]
            segs.append((a, blob[i + 5:i + 5 + n]))
            i += 5 + n
        elif kind == 0xFF:
            return segs, (blob[i + 3] << 8) | blob[i + 4]
        else:
            raise ValueError("bad ML block at %d" % i)
    return segs, None


def ml_file(addr, data, exec_addr):
    n = len(data)
    return bytes([0, n >> 8, n & 0xFF, addr >> 8, addr & 0xFF]) + bytes(data) + \
        bytes([0xFF, 0, 0, exec_addr >> 8, exec_addr & 0xFF])
