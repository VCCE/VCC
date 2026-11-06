// sdcharness - drive sdc.dll directly through its port interface, the
// same way VCC does, and replay the STRMTEST.BIN test sequence.
//
// Build (x86 Developer prompt):  cl /EHsc /O2 sdcharness.cpp
// Run:  sdcharness <path\to\sdc.dll> <sd card folder>
//
// The SD card folder must contain STRM512.DAT and STRMODD.DAT.

#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

using slot_id_type = std::size_t;
typedef void (*WriteMemCb)(slot_id_type, unsigned char, unsigned short);
typedef unsigned char (*ReadMemCb)(slot_id_type, unsigned short);
typedef void (*CartLineCb)(slot_id_type, bool);
typedef void (*AssertIntCb)(slot_id_type, int, int);
struct cpak_callbacks {
    AssertIntCb assert_interrupt;
    CartLineCb  assert_cartridge_line;
    WriteMemCb  write_memory_byte;
    ReadMemCb   read_memory_byte;
};

typedef void (*PakInitialize_t)(slot_id_type, const char*, HWND, const cpak_callbacks*);
typedef void (*PakReset_t)();
typedef void (*PakWritePort_t)(unsigned char, unsigned char);
typedef unsigned char (*PakReadPort_t)(unsigned char);

static PakWritePort_t wr;
static PakReadPort_t rd;
static int nmi_count = 0;

static void cb_int(slot_id_type, int i, int s) { nmi_count++; printf("    [interrupt %d src %d]\n", i, s); }
static void cb_line(slot_id_type, bool) {}
static void cb_wmem(slot_id_type, unsigned char, unsigned short) {}
static unsigned char cb_rmem(slot_id_type, unsigned short) { return 0; }

enum { RC_RDY, RC_END, RC_TO, RC_BAD, RC_RUN };
static unsigned char buf[512];

static unsigned char status() { return rd(0x48); }
static void cmdon() { wr(0x40, 0x43); }
static void cmdoff() { wr(0x40, 0x00); }

static bool waitnb(unsigned char& st) {
    for (int i = 0; i < 100000; i++) { st = status(); if (!(st & 1)) return true; }
    return false;
}
static int waitrdy(unsigned char& st) {
    for (int i = 0; i < 100000; i++) {
        st = status();
        if (!(st & 1)) return RC_END;
        if (st & 2) return RC_RDY;
    }
    return RC_TO;
}
static void expected(unsigned b, unsigned char* e) {
    e[0] = (b >> 16) & 0xFF; e[1] = (b >> 8) & 0xFF; e[2] = b & 0xFF; e[3] = ~b & 0xFF;
    for (int i = 4; i < 512; i++) e[i] = (b + i) & 0xFF;
}
// read a block like the 6809 code: ldu $FF4A (reads $FF4A then $FF4B)
static int readblk(bool bytemode, unsigned char& st) {
    int rc = waitrdy(st);
    if (rc != RC_RDY) return rc;
    if (bytemode) for (int i = 0; i < 512; i++) buf[i] = rd(0x4B);
    else for (int i = 0; i < 512; i += 2) { buf[i] = rd(0x4A); buf[i + 1] = rd(0x4B); }
    return RC_RDY;
}

static bool domount(const char* name) {
    unsigned char st, blk[256] = {};
    blk[0] = 'M'; blk[1] = ':';
    strcpy((char*)blk + 2, name);
    cmdon();
    if (!waitnb(st)) { printf("  mount: busy timeout st=%02X\n", st); return false; }
    wr(0x48, 0xE1);
    if (waitrdy(st) != RC_RDY) { printf("  mount: not ready st=%02X\n", st); return false; }
    for (int i = 0; i < 256; i += 2) { wr(0x4A, blk[i]); wr(0x4B, blk[i + 1]); }
    if (!waitnb(st)) { printf("  mount: done timeout st=%02X\n", st); return false; }
    printf("  mount %s st=%02X\n", name, st);
    return !(st & 0x80);
}

// returns rc; prints details
static int runstream(unsigned char cmd, unsigned start, int nread, int& cnt, unsigned char& fst) {
    unsigned char st, e[512];
    cnt = 0; fst = 0;
    cmdon();
    if (!waitnb(st)) { printf("  start: busy timeout st=%02X\n", st); return RC_TO; }
    wr(0x49, (start >> 16) & 0xFF); wr(0x4A, (start >> 8) & 0xFF); wr(0x4B, start & 0xFF);
    wr(0x48, cmd);
    for (unsigned b = start;; b++) {
        int rc = readblk(cmd & 4, st);
        if (rc == RC_END) { fst = st; return RC_END; }
        if (rc == RC_TO) { printf("  timeout st=%02X n=%d\n", st, cnt); return RC_TO; }
        expected(b, e);
        for (int i = 0; i < 512; i++) if (buf[i] != e[i]) {
            printf("  BAD blk=%u off=%d exp=%02X got=%02X data=%02X %02X %02X %02X\n",
                b, i, e[i], buf[i], buf[0], buf[1], buf[2], buf[3]);
            return RC_BAD;
        }
        cnt++;
        if (nread && cnt == nread) return RC_RDY;
        if (cnt > 68) return RC_RUN;
    }
}
static void doabort() { unsigned char st; wr(0x48, 0xD0); waitnb(st); printf("  abort st=%02X\n", st); }

int main(int argc, char** argv) {
    if (argc < 3) { printf("usage: sdcharness sdc.dll sdcard_dir\n"); return 1; }
    char ini[MAX_PATH];
    GetTempPathA(MAX_PATH, ini);
    strcat(ini, "sdcharness.ini");
    std::string cfg = std::string("[SDC]\r\nSDCardPath=") + argv[2] + "\r\nClockEnable=0\r\n";
    FILE* f = fopen(ini, "wb"); fwrite(cfg.data(), 1, cfg.size(), f); fclose(f);

    HMODULE h = LoadLibraryA(argv[1]);
    if (!h) { printf("LoadLibrary failed %lu\n", GetLastError()); return 1; }
    auto init = (PakInitialize_t)GetProcAddress(h, "PakInitialize");
    auto reset = (PakReset_t)GetProcAddress(h, "PakReset");
    wr = (PakWritePort_t)GetProcAddress(h, "PakWritePort");
    rd = (PakReadPort_t)GetProcAddress(h, "PakReadPort");
    if (!init || !reset || !wr || !rd) { printf("missing exports\n"); return 1; }
    static cpak_callbacks cb = { cb_int, cb_line, cb_wmem, cb_rmem };
    init(1, ini, nullptr, &cb);
    reset();

    int cnt; unsigned char fst, st;
    printf("T1 word stream 0..end\n");
    if (domount("STRM512.DAT")) {
        int rc = runstream(0x91, 0, 0, cnt, fst);
        printf("  rc=%d n=%d fin=%02X\n", rc, cnt, fst);
    }
    cmdoff();
    printf("T2 blk 10 x5 + abort\n");
    printf("  rc=%d\n", runstream(0x91, 10, 5, cnt, fst)); doabort(); cmdoff();
    printf("T3 blk 3 x2 after abort\n");
    printf("  rc=%d\n", runstream(0x91, 3, 2, cnt, fst)); doabort(); cmdoff();
    printf("T4 latch exit mid-stream\n");
    printf("  rc=%d\n", runstream(0x91, 30, 2, cnt, fst)); cmdoff();
    printf("  rc=%d\n", runstream(0x91, 7, 1, cnt, fst)); doabort(); cmdoff();
    printf("T5 byte mode blk 5 x3\n");
    printf("  rc=%d\n", runstream(0x95, 5, 3, cnt, fst)); doabort(); cmdoff();
    printf("T6 sector read LSN 2\n");
    cmdon(); waitnb(st);
    wr(0x49, 0); wr(0x4A, 0); wr(0x4B, 2); wr(0x48, 0x81);
    {
        int rc = waitrdy(st);
        printf("  ready rc=%d st=%02X\n", rc, st);
        if (rc == RC_RDY) {
            for (int i = 0; i < 256; i += 2) { buf[i] = rd(0x4A); buf[i + 1] = rd(0x4B); }
            unsigned char e[512]; expected(1, e);
            printf("  data %s\n", memcmp(buf, e, 256) ? "BAD" : "ok");
            waitnb(st); printf("  fin st=%02X\n", st);
        }
    }
    cmdoff();
    printf("T7 odd size file\n");
    if (domount("STRMODD.DAT")) {
        int rc = runstream(0x91, 0, 64, cnt, fst);
        printf("  rc=%d n=%d fin=%02X\n", rc, cnt, fst);
        rc = readblk(false, st);
        printf("  partial rc=%d st=%02X hdr=%02X %02X %02X\n", rc, st, buf[0], buf[1], buf[2]);
        rc = readblk(false, st);
        printf("  end rc=%d st=%02X\n", rc, st);
    }
    cmdoff();
    printf("T8 start past end\n");
    printf("  rc=%d fin=%02X\n", runstream(0x91, 69, 1, cnt, fst), fst); doabort(); cmdoff();
    // T9: NTM player pattern - sector counter kept in $FF42 while streaming.
    // STA <$42 ; loop { read 512 ; poll ; DEC <$42 ; BNE loop }
    printf("T9 $FF42 counter during stream\n");
    if (domount("STRM512.DAT")) {
        cmdon(); waitnb(st);
        wr(0x49, 0); wr(0x4A, 0); wr(0x4B, 20); wr(0x48, 0x91);
        int ok = 1, blocks = 0;
        for (int pass = 0; pass < 3 && ok; pass++) {
            wr(0x42, 4);                                  // STA <$42
            unsigned char ctr;
            do {
                int rc = readblk(false, st);
                if (rc != RC_RDY) { printf("  rc=%d st=%02X\n", rc, st); ok = 0; break; }
                unsigned char e[512]; expected(20 + blocks, e);
                if (memcmp(buf, e, 512)) { printf("  BAD blk %d got %02X%02X%02X\n", 20 + blocks, buf[0], buf[1], buf[2]); ok = 0; break; }
                blocks++;
                ctr = rd(0x42) - 1; wr(0x42, ctr);        // DEC <$42
            } while (ctr != 0 && blocks < 40);
        }
        printf("  blocks=%d (expect 12) %s\n", blocks, (ok && blocks == 12) ? "PASS" : "FAIL");
        wr(0x48, 0xD0); waitnb(st);
        cmdoff();
        wr(0x42, 0x5A);
        printf("  floppy mode $FF42 readback=%02X %s\n", rd(0x42), rd(0x42) == 0x5A ? "PASS" : "FAIL");
    }
    // T10: CSM player pattern - after each READY poll the player writes a
    // word to $FF4A (STD <$4A) before reading the block. Writes during a
    // stream must be ignored.
    printf("T10 write $FF4A during stream\n");
    if (domount("STRM512.DAT")) {
        cmdon(); waitnb(st);
        wr(0x49, 0); wr(0x4A, 0); wr(0x4B, 0); wr(0x48, 0x91);
        int n = 0, bad = 0;
        for (;;) {
            int rc = waitrdy(st);
            if (rc != RC_RDY) break;
            wr(0x4A, 0x12); wr(0x4B, 0x34);               // STD <$4A
            for (int i = 0; i < 512; i += 2) { buf[i] = rd(0x4A); buf[i + 1] = rd(0x4B); }
            unsigned char e[512]; expected(n, e);
            if (memcmp(buf, e, 512)) {
                printf("  BAD blk %d got %02X%02X%02X%02X\n", n, buf[0], buf[1], buf[2], buf[3]);
                bad = 1; break;
            }
            if (++n > 70) break;
        }
        printf("  blocks=%d fin=%02X %s\n", n, st, (!bad && n == 64 && st == 0) ? "PASS" : "FAIL");
        wr(0x48, 0xD0); waitnb(st); cmdoff();
    }
    printf("interrupts asserted: %d\n", nmi_count);
    return 0;
}
