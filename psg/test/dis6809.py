# dis6809.py -- small linear-sweep 6809 disassembler for reading the PSG firmware.
#   python dis6809.py FILE OFFSET LENGTH ORG
import sys

R4 = ["X", "Y", "U", "S"]
TFR = {0: "D", 1: "X", 2: "Y", 3: "U", 4: "S", 5: "PC", 8: "A", 9: "B", 10: "CC", 11: "DP"}
BR = ["BRA", "BRN", "BHI", "BLS", "BCC", "BCS", "BNE", "BEQ",
      "BVC", "BVS", "BPL", "BMI", "BGE", "BLT", "BGT", "BLE"]
RMW = {0x0: "NEG", 0x3: "COM", 0x4: "LSR", 0x6: "ROR", 0x7: "ASR", 0x8: "ASL",
       0x9: "ROL", 0xA: "DEC", 0xC: "INC", 0xD: "TST", 0xE: "JMP", 0xF: "CLR"}
ACC_A = {0x0: "SUBA", 0x1: "CMPA", 0x2: "SBCA", 0x3: "SUBD", 0x4: "ANDA", 0x5: "BITA",
         0x6: "LDA", 0x7: "STA", 0x8: "EORA", 0x9: "ADCA", 0xA: "ORA", 0xB: "ADDA",
         0xC: "CMPX", 0xD: "JSR", 0xE: "LDX", 0xF: "STX"}
ACC_B = {0x0: "SUBB", 0x1: "CMPB", 0x2: "SBCB", 0x3: "ADDD", 0x4: "ANDB", 0x5: "BITB",
         0x6: "LDB", 0x7: "STB", 0x8: "EORB", 0x9: "ADCB", 0xA: "ORB", 0xB: "ADDB",
         0xC: "LDD", 0xD: "STD", 0xE: "LDU", 0xF: "STU"}
WIDE = {"SUBD", "CMPX", "LDX", "ADDD", "LDD", "LDU", "CMPD", "CMPY", "LDY", "LDS",
        "CMPU", "CMPS"}
P2 = {0x3: "CMPD", 0xC: "CMPY", 0xE: "LDY", 0xF: "STY"}
P2B = {0xE: "LDS", 0xF: "STS"}
P3 = {0x3: "CMPU", 0xC: "CMPS"}
INH = {0x12: "NOP", 0x13: "SYNC", 0x19: "DAA", 0x1D: "SEX", 0x39: "RTS", 0x3A: "ABX",
       0x3B: "RTI", 0x3D: "MUL", 0x3F: "SWI"}


class Dis:
    def __init__(self, data, org):
        self.d, self.org = data, org

    def b(self, pc):
        return self.d[pc - self.org] if 0 <= pc - self.org < len(self.d) else 0

    def w(self, pc):
        return (self.b(pc) << 8) | self.b(pc + 1)

    def idx(self, pc):
        pb = self.b(pc)
        r = R4[(pb >> 5) & 3]
        if not pb & 0x80:
            off = pb & 0x1F
            off = off - 32 if off & 0x10 else off
            return "%d,%s" % (off, r), 1
        ind, m = pb & 0x10, pb & 0x0F
        n = 1
        if m == 0: s = ",%s+" % r
        elif m == 1: s = ",%s++" % r
        elif m == 2: s = ",-%s" % r
        elif m == 3: s = ",--%s" % r
        elif m == 4: s = ",%s" % r
        elif m == 5: s = "B,%s" % r
        elif m == 6: s = "A,%s" % r
        elif m == 8:
            o = self.b(pc + 1); o = o - 256 if o & 0x80 else o
            s = "%d,%s" % (o, r); n = 2
        elif m == 9:
            o = self.w(pc + 1); o = o - 65536 if o & 0x8000 else o
            s = "%d,%s" % (o, r); n = 3
        elif m == 0xB: s = "D,%s" % r
        elif m == 0xC:
            o = self.b(pc + 1); o = o - 256 if o & 0x80 else o
            s = "$%04X,PCR" % ((pc + 2 + o) & 0xFFFF); n = 2
        elif m == 0xD:
            o = self.w(pc + 1)
            s = "$%04X,PCR" % ((pc + 3 + o) & 0xFFFF); n = 3
        elif m == 0xF:
            return "[$%04X]" % self.w(pc + 1), 3
        else:
            s = "?idx%02X" % pb
        return ("[%s]" % s) if ind else s, n

    def operand(self, name, mode, pc):
        if mode == 0:  # immediate
            if name in WIDE:
                return "#$%04X" % self.w(pc), 2
            return "#$%02X" % self.b(pc), 1
        if mode == 1:
            return "<$%02X" % self.b(pc), 1
        if mode == 2:
            return self.idx(pc)
        return "$%04X" % self.w(pc), 2

    def one(self, pc):
        op = self.b(pc)
        hi, lo = op >> 4, op & 0xF
        if op in (0x10, 0x11):
            op2 = self.b(pc + 1)
            h2, l2 = op2 >> 4, op2 & 0xF
            if op == 0x10 and h2 == 2 and l2:
                return "L" + BR[l2] + " $%04X" % ((pc + 4 + self.w(pc + 2)) & 0xFFFF), 4
            if op2 == 0x3F:
                return ("SWI2" if op == 0x10 else "SWI3"), 2
            if h2 >= 8:
                mode = (h2 - 8) & 3
                if op == 0x10:
                    name = (P2 if h2 < 0xC else P2B).get(l2)
                else:
                    name = P3.get(l2) if h2 < 0xC else None
                if name and not (mode == 0 and name.startswith("ST")):
                    s, n = self.operand(name, mode, pc + 2)
                    return "%s %s" % (name, s), 2 + n
            return "FCB $%02X,$%02X" % (op, op2), 2
        if op in INH:
            return INH[op], 1
        if op == 0x16:
            return "LBRA $%04X" % ((pc + 3 + self.w(pc + 1)) & 0xFFFF), 3
        if op == 0x17:
            return "LBSR $%04X" % ((pc + 3 + self.w(pc + 1)) & 0xFFFF), 3
        if op in (0x1A, 0x1C, 0x3C):
            return {0x1A: "ORCC", 0x1C: "ANDCC", 0x3C: "CWAI"}[op] + " #$%02X" % self.b(pc + 1), 2
        if op in (0x1E, 0x1F):
            pb = self.b(pc + 1)
            return "%s %s,%s" % ("EXG" if op == 0x1E else "TFR",
                                 TFR.get(pb >> 4, "?"), TFR.get(pb & 15, "?")), 2
        if hi == 2:
            o = self.b(pc + 1); o = o - 256 if o & 0x80 else o
            return "%s $%04X" % (BR[lo], (pc + 2 + o) & 0xFFFF), 2
        if 0x30 <= op <= 0x33:
            s, n = self.idx(pc + 1)
            return "LEA%s %s" % ("XYSU"[lo], s), 1 + n
        if 0x34 <= op <= 0x37:
            pb = self.b(pc + 1)
            other = "U" if op < 0x36 else "S"
            names = ["CC", "A", "B", "DP", "X", "Y", other, "PC"]
            regs = ",".join(names[i] for i in range(8) if pb >> i & 1)
            return "%s %s" % (["PSHS", "PULS", "PSHU", "PULU"][op - 0x34], regs), 2
        if hi in (4, 5) and lo in RMW and lo != 0xE:
            return RMW[lo] + ("A" if hi == 4 else "B"), 1
        if hi in (0, 6, 7) and lo in RMW:
            mode = {0: 1, 6: 2, 7: 3}[hi]
            s, n = self.operand(RMW[lo], mode, pc + 1)
            return "%s %s" % (RMW[lo], s), 1 + n
        if hi >= 8:
            table = ACC_A if hi < 0xC else ACC_B
            name = table[lo]
            mode = (hi - 8) & 3
            if op == 0x8D:
                o = self.b(pc + 1); o = o - 256 if o & 0x80 else o
                return "BSR $%04X" % ((pc + 2 + o) & 0xFFFF), 2
            if mode == 0 and (name.startswith("ST") or name == "JSR"):
                return "FCB $%02X" % op, 1
            s, n = self.operand(name, mode, pc + 1)
            return "%s %s" % (name, s), 1 + n
        return "FCB $%02X" % op, 1

    def run(self, start=None, end=None):
        pc = self.org if start is None else start
        end = self.org + len(self.d) if end is None else end
        out = []
        while pc < end:
            text, n = self.one(pc)
            raw = " ".join("%02X" % self.b(pc + i) for i in range(n))
            out.append("%04X  %-14s %s" % (pc, raw, text))
            pc += n
        return out


if __name__ == "__main__":
    f, off, ln, org = sys.argv[1], int(sys.argv[2], 0), int(sys.argv[3], 0), int(sys.argv[4], 0)
    data = open(f, "rb").read()[off:off + ln]
    print("\n".join(Dis(data, org).run()))
