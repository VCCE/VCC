// sp0256.cpp
// See sp0256.h for provenance/license notes. Mechanical port of MAME's
// sp0256.cpp LPC-12 filter and microsequencer to a standalone class.

#include "sp0256.h"

namespace ssc {

Sp0256::Sp0256()
{
    m_rom = std::make_unique<uint8_t[]>(0x10000);
    std::memset(m_rom.get(), 0, 0x10000);
    m_scratch = std::make_unique<int16_t[]>(kScbufSize);
    std::memset(m_scratch.get(), 0, kScbufSize * sizeof(int16_t));
    m_filt.rng = 1;
}

void Sp0256::LoadRom(const uint8_t* al2_2k, size_t len)
{
    std::memset(m_rom.get(), 0, 0x10000);
    if (al2_2k && len) {
        size_t n = len < 0x800 ? len : 0x800;
        std::memcpy(m_rom.get() + 0x1000, al2_2k, n);
    }
}

void Sp0256::SetSby(int line_state)
{
    if (m_sby_line != line_state) {
        m_sby_line = line_state;
    }
}

void Sp0256::Reset()
{
    m_fifo_head = m_fifo_tail = m_fifo_bitp = 0;
    m_filt = Lpc12{};
    m_filt.rng = 1;
    m_halted   = 1;
    m_filt.rpt = -1;
    m_lrq      = 1;
    m_ald      = 0;
    m_pc       = 0;
    m_stack    = 0;
    m_fifo_sel = 0;
    m_mode     = 0;
    m_page     = 0x1000 << 3;
    m_silent   = 1;
    m_sby_line = 0;
    if (DataRequestChanged) DataRequestChanged(true);
    SetSby(1);
}

// ------------------------------------------------------------------
// LIMIT / LPC12 update -- ported verbatim (algorithmically) from MAME
// ------------------------------------------------------------------

int16_t Sp0256::Lpc12::Limit(int16_t s)
{
    if (s >  8191) return  8191;
    if (s < -8192) return -8192;
    return s;
}

int Sp0256::Lpc12::Update(int num_samp, int16_t* out, uint32_t* optr)
{
    int i;
    int oidx = int(*optr);

    for (i = 0; i < num_samp; i++)
    {
        bool do_int = false;
        uint16_t samp = 0;
        if (per)
        {
            if (cnt <= 0)
            {
                cnt += per;
                samp = uint16_t(amp);
                rpt--;
                do_int = interp != 0;
                for (int j = 0; j < 6; j++) z_data[j][1] = z_data[j][0] = 0;
            }
            else
            {
                samp = 0;
                cnt--;
            }
        }
        else
        {
            if (--cnt <= 0)
            {
                do_int = interp != 0;
                cnt = kPerNoise;
                rpt--;
                for (int j = 0; j < 6; j++) z_data[j][0] = z_data[j][1] = 0;
            }

            const bool bit = (rng & 1) != 0;
            rng = (rng >> 1) ^ (bit ? 0x4001u : 0u);
            samp = bit ? uint16_t(amp) : uint16_t(-amp);
        }

        if (do_int)
        {
            r[0] = uint8_t(r[0] + r[14]);
            r[1] = uint8_t(r[1] + r[15]);
            amp   = (r[0] & 0x1F) << (((r[0] & 0xE0) >> 5) + 0);
            per   = r[1];
        }

        if (rpt <= 0) break;

        for (int j = 0; j < 6; j++)
        {
            int s = int16_t(samp);
            s += (int(b_coef[j]) * int(z_data[j][1])) >> 9;
            s += (int(f_coef[j]) * int(z_data[j][0])) >> 8;
            samp = uint16_t(s);
            z_data[j][1] = z_data[j][0];
            z_data[j][0] = int16_t(samp);
        }

        out[oidx++ & kScbufMask] = int16_t(Limit(int16_t(samp)) << 2);
    }

    *optr = uint32_t(oidx);
    return i;
}

void Sp0256::Lpc12::Regdec()
{
    amp = (r[0] & 0x1F) << (((r[0] & 0xE0) >> 5) + 0);
    cnt = 0;
    per = r[1];

    static constexpr int16_t qtbl[128] = {
        0,      9,      17,     25,     33,     41,     49,     57,
        65,     73,     81,     89,     97,     105,    113,    121,
        129,    137,    145,    153,    161,    169,    177,    185,
        193,    201,    209,    217,    225,    233,    241,    249,
        257,    265,    273,    281,    289,    297,    301,    305,
        309,    313,    317,    321,    325,    329,    333,    337,
        341,    345,    349,    353,    357,    361,    365,    369,
        373,    377,    381,    385,    389,    393,    397,    401,
        405,    409,    413,    417,    421,    425,    427,    429,
        431,    433,    435,    437,    439,    441,    443,    445,
        447,    449,    451,    453,    455,    457,    459,    461,
        463,    465,    467,    469,    471,    473,    475,    477,
        479,    481,    482,    483,    484,    485,    486,    487,
        488,    489,    490,    491,    492,    493,    494,    495,
        496,    497,    498,    499,    500,    501,    502,    503,
        504,    505,    506,    507,    508,    509,    510,    511
    };

    auto IQ = [](uint8_t x) -> int16_t {
        return (x & 0x80) ? int16_t(qtbl[0x7F & uint8_t(-x)]) : int16_t(-qtbl[x]);
    };

    for (int i = 0; i < 6; i++) {
        b_coef[i] = IQ(r[2 + 2 * i]);
        f_coef[i] = IQ(r[3 + 2 * i]);
    }

    interp = (r[14] || r[15]) ? 1 : 0;
}

// ------------------------------------------------------------------
// SP0256 data-format tables -- verbatim from MAME (see original for
// bitfield documentation: len/shift/param/delta/field/clr5/clrall).
// ------------------------------------------------------------------

#define CR(l,s,p,d,f,c5,ca)         \
        (                           \
            (((l)  & 15) <<  0) |   \
            (((s)  & 15) <<  4) |   \
            (((p)  & 15) <<  8) |   \
            (((d)  &  1) << 12) |   \
            (((f)  &  1) << 13) |   \
            (((c5) &  1) << 14) |   \
            (((ca) &  1) << 15)     \
        )

#define CR_DELTA  CR(0,0,0,1,0,0,0)
#define CR_FIELD  CR(0,0,0,0,1,0,0)
#define CR_CLR5   CR(0,0,0,0,0,1,0)
#define CR_CLRA   CR(0,0,0,0,0,0,1)
#define CR_LEN(x) ((x) & 15)
#define CR_SHF(x) (((x) >> 4) & 15)
#define CR_PRM(x) (((x) >> 8) & 15)

enum { AM = 0, PR, B0, F0, B1, F1, B2, F2, B3, F3, B4, F4, B5, F5, IA, IP };

static const uint16_t sp0256_datafmt[] =
{
    /*    0 */  CR( 0,  0,  0,  0,  0,  0,  1),
    /*    1 */  CR( 8,  0,  AM, 0,  0,  0,  1),
    /*    2 */  CR( 8,  0,  PR, 0,  0,  0,  0),
    /*    3 */  CR( 8,  0,  B0, 0,  0,  0,  0),
    /*    4 */  CR( 8,  0,  F0, 0,  0,  0,  0),
    /*    5 */  CR( 8,  0,  B1, 0,  0,  0,  0),
    /*    6 */  CR( 8,  0,  F1, 0,  0,  0,  0),
    /*    7 */  CR( 8,  0,  B2, 0,  0,  0,  0),
    /*    8 */  CR( 8,  0,  F2, 0,  0,  0,  0),
    /*    9 */  CR( 8,  0,  B3, 0,  0,  0,  0),
    /*   10 */  CR( 8,  0,  F3, 0,  0,  0,  0),
    /*   11 */  CR( 8,  0,  B4, 0,  0,  0,  0),
    /*   12 */  CR( 8,  0,  F4, 0,  0,  0,  0),
    /*   13 */  CR( 8,  0,  B5, 0,  0,  0,  0),
    /*   14 */  CR( 8,  0,  F5, 0,  0,  0,  0),
    /*   15 */  CR( 8,  0,  IA, 0,  0,  0,  0),
    /*   16 */  CR( 8,  0,  IP, 0,  0,  0,  0),

    /*   17 */  CR( 6,  2,  AM, 0,  0,  0,  1),
    /*   18 */  CR( 8,  0,  PR, 0,  0,  0,  0),
    /*   19 */  CR( 4,  3,  B3, 0,  0,  0,  0),
    /*   20 */  CR( 6,  2,  F3, 0,  0,  0,  0),
    /*   21 */  CR( 7,  1,  B4, 0,  0,  0,  0),
    /*   22 */  CR( 6,  2,  F4, 0,  0,  0,  0),
    /*   23 */  CR( 8,  0,  B5, 0,  0,  0,  0),
    /*   24 */  CR( 8,  0,  F5, 0,  0,  0,  0),

    /*   25 */  CR( 6,  2,  AM, 0,  0,  0,  1),
    /*   26 */  CR( 8,  0,  PR, 0,  0,  0,  0),
    /*   27 */  CR( 6,  1,  B3, 0,  0,  0,  0),
    /*   28 */  CR( 7,  1,  F3, 0,  0,  0,  0),
    /*   29 */  CR( 8,  0,  B4, 0,  0,  0,  0),
    /*   30 */  CR( 8,  0,  F4, 0,  0,  0,  0),
    /*   31 */  CR( 8,  0,  B5, 0,  0,  0,  0),
    /*   32 */  CR( 8,  0,  F5, 0,  0,  0,  0),

    /*   33 */  CR( 0,  0,  0,  0,  0,  1,  0),
    /*   34 */  CR( 6,  2,  AM, 0,  0,  0,  0),
    /*   35 */  CR( 6,  2,  F3, 0,  1,  0,  0),
    /*   36 */  CR( 6,  2,  F4, 0,  1,  0,  0),
    /*   37 */  CR( 8,  0,  F5, 0,  1,  0,  0),

    /*   38 */  CR( 0,  0,  0,  0,  0,  1,  0),
    /*   39 */  CR( 6,  2,  AM, 0,  0,  0,  0),
    /*   40 */  CR( 7,  1,  F3, 0,  1,  0,  0),
    /*   41 */  CR( 8,  0,  F4, 0,  1,  0,  0),
    /*   42 */  CR( 8,  0,  F5, 0,  1,  0,  0),

    /*   43 */  0,
    /*   44 */  0,

    /*   45 */  CR( 4,  2,  AM, 1,  0,  0,  0),
    /*   46 */  CR( 5,  0,  PR, 1,  0,  0,  0),
    /*   47 */  CR( 3,  4,  B0, 1,  0,  0,  0),
    /*   48 */  CR( 3,  3,  F0, 1,  0,  0,  0),
    /*   49 */  CR( 3,  4,  B1, 1,  0,  0,  0),
    /*   50 */  CR( 3,  3,  F1, 1,  0,  0,  0),
    /*   51 */  CR( 3,  4,  B2, 1,  0,  0,  0),
    /*   52 */  CR( 3,  3,  F2, 1,  0,  0,  0),
    /*   53 */  CR( 3,  3,  B3, 1,  0,  0,  0),
    /*   54 */  CR( 4,  2,  F3, 1,  0,  0,  0),
    /*   55 */  CR( 4,  1,  B4, 1,  0,  0,  0),
    /*   56 */  CR( 4,  2,  F4, 1,  0,  0,  0),
    /*   57 */  CR( 5,  0,  B5, 1,  0,  0,  0),
    /*   58 */  CR( 5,  0,  F5, 1,  0,  0,  0),

    /*   59 */  CR( 4,  2,  AM, 1,  0,  0,  0),
    /*   60 */  CR( 5,  0,  PR, 1,  0,  0,  0),
    /*   61 */  CR( 4,  1,  B0, 1,  0,  0,  0),
    /*   62 */  CR( 4,  2,  F0, 1,  0,  0,  0),
    /*   63 */  CR( 4,  1,  B1, 1,  0,  0,  0),
    /*   64 */  CR( 4,  2,  F1, 1,  0,  0,  0),
    /*   65 */  CR( 4,  1,  B2, 1,  0,  0,  0),
    /*   66 */  CR( 4,  2,  F2, 1,  0,  0,  0),
    /*   67 */  CR( 4,  1,  B3, 1,  0,  0,  0),
    /*   68 */  CR( 5,  1,  F3, 1,  0,  0,  0),
    /*   69 */  CR( 5,  0,  B4, 1,  0,  0,  0),
    /*   70 */  CR( 5,  0,  F4, 1,  0,  0,  0),
    /*   71 */  CR( 5,  0,  B5, 1,  0,  0,  0),
    /*   72 */  CR( 5,  0,  F5, 1,  0,  0,  0),

    /*   73 */  CR( 0,  0,  0,  0,  0,  1,  0),
    /*   74 */  CR( 6,  2,  AM, 0,  0,  0,  0),
    /*   75 */  CR( 5,  3,  F0, 0,  1,  0,  0),
    /*   76 */  CR( 5,  3,  F1, 0,  1,  0,  0),
    /*   77 */  CR( 5,  3,  F2, 0,  1,  0,  0),

    /*   78 */  CR( 0,  0,  0,  0,  0,  1,  0),
    /*   79 */  CR( 6,  2,  AM, 0,  0,  0,  0),
    /*   80 */  CR( 6,  2,  F0, 0,  1,  0,  0),
    /*   81 */  CR( 6,  2,  F1, 0,  1,  0,  0),
    /*   82 */  CR( 6,  2,  F2, 0,  1,  0,  0),

    /*   83 */  CR( 6,  2,  AM, 0,  0,  0,  1),
    /*   84 */  CR( 8,  0,  PR, 0,  0,  0,  0),
    /*   85 */  CR( 3,  4,  B0, 0,  0,  0,  0),
    /*   86 */  CR( 5,  3,  F0, 0,  0,  0,  0),
    /*   87 */  CR( 3,  4,  B1, 0,  0,  0,  0),
    /*   88 */  CR( 5,  3,  F1, 0,  0,  0,  0),
    /*   89 */  CR( 3,  4,  B2, 0,  0,  0,  0),
    /*   90 */  CR( 5,  3,  F2, 0,  0,  0,  0),
    /*   91 */  CR( 4,  3,  B3, 0,  0,  0,  0),
    /*   92 */  CR( 6,  2,  F3, 0,  0,  0,  0),
    /*   93 */  CR( 7,  1,  B4, 0,  0,  0,  0),
    /*   94 */  CR( 6,  2,  F4, 0,  0,  0,  0),
    /*   95 */  CR( 5,  0,  IA, 0,  0,  0,  0),
    /*   96 */  CR( 5,  0,  IP, 0,  0,  0,  0),

    /*   97 */  CR( 6,  2,  AM, 0,  0,  0,  1),
    /*   98 */  CR( 8,  0,  PR, 0,  0,  0,  0),
    /*   99 */  CR( 6,  1,  B0, 0,  0,  0,  0),
    /*  100 */  CR( 6,  2,  F0, 0,  0,  0,  0),
    /*  101 */  CR( 6,  1,  B1, 0,  0,  0,  0),
    /*  102 */  CR( 6,  2,  F1, 0,  0,  0,  0),
    /*  103 */  CR( 6,  1,  B2, 0,  0,  0,  0),
    /*  104 */  CR( 6,  2,  F2, 0,  0,  0,  0),
    /*  105 */  CR( 6,  1,  B3, 0,  0,  0,  0),
    /*  106 */  CR( 7,  1,  F3, 0,  0,  0,  0),
    /*  107 */  CR( 8,  0,  B4, 0,  0,  0,  0),
    /*  108 */  CR( 8,  0,  F4, 0,  0,  0,  0),
    /*  109 */  CR( 5,  0,  IA, 0,  0,  0,  0),
    /*  110 */  CR( 5,  0,  IP, 0,  0,  0,  0),

    /*  111 */  CR( 4,  2,  AM, 1,  0,  0,  0),
    /*  112 */  CR( 5,  0,  PR, 1,  0,  0,  0),
    /*  113 */  CR( 3,  3,  B3, 1,  0,  0,  0),
    /*  114 */  CR( 4,  2,  F3, 1,  0,  0,  0),
    /*  115 */  CR( 4,  1,  B4, 1,  0,  0,  0),
    /*  116 */  CR( 4,  2,  F4, 1,  0,  0,  0),
    /*  117 */  CR( 5,  0,  B5, 1,  0,  0,  0),
    /*  118 */  CR( 5,  0,  F5, 1,  0,  0,  0),

    /*  119 */  CR( 4,  2,  AM, 1,  0,  0,  0),
    /*  120 */  CR( 5,  0,  PR, 1,  0,  0,  0),
    /*  121 */  CR( 4,  1,  B3, 1,  0,  0,  0),
    /*  122 */  CR( 5,  1,  F3, 1,  0,  0,  0),
    /*  123 */  CR( 5,  0,  B4, 1,  0,  0,  0),
    /*  124 */  CR( 5,  0,  F4, 1,  0,  0,  0),
    /*  125 */  CR( 5,  0,  B5, 1,  0,  0,  0),
    /*  126 */  CR( 5,  0,  F5, 1,  0,  0,  0),

    /*  127 */  CR( 6,  2,  AM, 0,  0,  0,  0),
    /*  128 */  CR( 8,  0,  PR, 0,  0,  0,  0),

    /*  129 */  CR( 6,  2,  AM, 0,  0,  0,  1),
    /*  130 */  CR( 8,  0,  PR, 0,  0,  0,  0),
    /*  131 */  CR( 3,  4,  B0, 0,  0,  0,  0),
    /*  132 */  CR( 5,  3,  F0, 0,  0,  0,  0),
    /*  133 */  CR( 3,  4,  B1, 0,  0,  0,  0),
    /*  134 */  CR( 5,  3,  F1, 0,  0,  0,  0),
    /*  135 */  CR( 3,  4,  B2, 0,  0,  0,  0),
    /*  136 */  CR( 5,  3,  F2, 0,  0,  0,  0),
    /*  137 */  CR( 4,  3,  B3, 0,  0,  0,  0),
    /*  138 */  CR( 6,  2,  F3, 0,  0,  0,  0),
    /*  139 */  CR( 7,  1,  B4, 0,  0,  0,  0),
    /*  140 */  CR( 6,  2,  F4, 0,  0,  0,  0),
    /*  141 */  CR( 8,  0,  B5, 0,  0,  0,  0),
    /*  142 */  CR( 8,  0,  F5, 0,  0,  0,  0),
    /*  143 */  CR( 5,  0,  IA, 0,  0,  0,  0),
    /*  144 */  CR( 5,  0,  IP, 0,  0,  0,  0),

    /*  145 */  CR( 6,  2,  AM, 0,  0,  0,  1),
    /*  146 */  CR( 8,  0,  PR, 0,  0,  0,  0),
    /*  147 */  CR( 6,  1,  B0, 0,  0,  0,  0),
    /*  148 */  CR( 6,  2,  F0, 0,  0,  0,  0),
    /*  149 */  CR( 6,  1,  B1, 0,  0,  0,  0),
    /*  150 */  CR( 6,  2,  F1, 0,  0,  0,  0),
    /*  151 */  CR( 6,  1,  B2, 0,  0,  0,  0),
    /*  152 */  CR( 6,  2,  F2, 0,  0,  0,  0),
    /*  153 */  CR( 6,  1,  B3, 0,  0,  0,  0),
    /*  154 */  CR( 7,  1,  F3, 0,  0,  0,  0),
    /*  155 */  CR( 8,  0,  B4, 0,  0,  0,  0),
    /*  156 */  CR( 8,  0,  F4, 0,  0,  0,  0),
    /*  157 */  CR( 8,  0,  B5, 0,  0,  0,  0),
    /*  158 */  CR( 8,  0,  F5, 0,  0,  0,  0),
    /*  159 */  CR( 5,  0,  IA, 0,  0,  0,  0),
    /*  160 */  CR( 5,  0,  IP, 0,  0,  0,  0),

    /*  161 */  CR( 0,  0,  0,  0,  0,  1,  0),
    /*  162 */  CR( 6,  2,  AM, 0,  0,  0,  0),
    /*  163 */  CR( 8,  0,  PR, 0,  0,  0,  0),
    /*  164 */  CR( 5,  3,  F0, 0,  1,  0,  0),
    /*  165 */  CR( 5,  3,  F1, 0,  1,  0,  0),
    /*  166 */  CR( 5,  3,  F2, 0,  1,  0,  0),
    /*  167 */  CR( 5,  0,  IA, 0,  0,  0,  0),
    /*  168 */  CR( 5,  0,  IP, 0,  0,  0,  0),

    /*  169 */  CR( 0,  0,  0,  0,  0,  1,  0),
    /*  170 */  CR( 6,  2,  AM, 0,  0,  0,  0),
    /*  171 */  CR( 8,  0,  PR, 0,  0,  0,  0),
    /*  172 */  CR( 6,  2,  F0, 0,  1,  0,  0),
    /*  173 */  CR( 6,  2,  F1, 0,  1,  0,  0),
    /*  174 */  CR( 6,  2,  F2, 0,  1,  0,  0),
    /*  175 */  CR( 5,  0,  IA, 0,  0,  0,  0),
    /*  176 */  CR( 5,  0,  IP, 0,  0,  0,  0),
};

static const int16_t sp0256_df_idx[16 * 8] =
{
    /*  OPCODE 0000 */      -1, -1,     -1, -1,     -1, -1,     -1, -1,
    /*  OPCODE 1000 */      -1, -1,     -1, -1,     -1, -1,     -1, -1,
    /*  OPCODE 0100 */      17, 22,     17, 24,     25, 30,     25, 32,
    /*  OPCODE 1100 */      83, 94,     129,142,    97, 108,    145,158,
    /*  OPCODE 0010 */      83, 96,     129,144,    97, 110,    145,160,
    /*  OPCODE 1010 */      73, 77,     74, 77,     78, 82,     79, 82,
    /*  OPCODE 0110 */      33, 36,     34, 37,     38, 41,     39, 42,
    /*  OPCODE 1110 */      127,128,    127,128,    127,128,    127,128,
    /*  OPCODE 0001 */      1,  14,     1,  16,     1,  14,     1,  16,
    /*  OPCODE 1001 */      45, 56,     45, 58,     59, 70,     59, 72,
    /*  OPCODE 0101 */      161,166,    162,166,    169,174,    170,174,
    /*  OPCODE 1101 */      111,116,    111,118,    119,124,    119,126,
    /*  OPCODE 0011 */      161,168,    162,168,    169,176,    170,176,
    /*  OPCODE 1011 */      -1, -1,     -1, -1,     -1, -1,     -1, -1,
    /*  OPCODE 0111 */      -1, -1,     -1, -1,     -1, -1,     -1, -1,
    /*  OPCODE 1111 */      0,  0,      0,  0,      0,  0,      0,  0
};

static inline uint32_t bitrev32(uint32_t val)
{
    val = ((val & 0xFFFF0000) >> 16) | ((val & 0x0000FFFF) << 16);
    val = ((val & 0xFF00FF00) >>  8) | ((val & 0x00FF00FF) <<  8);
    val = ((val & 0xF0F0F0F0) >>  4) | ((val & 0x0F0F0F0F) <<  4);
    val = ((val & 0xCCCCCCCC) >>  2) | ((val & 0x33333333) <<  2);
    val = ((val & 0xAAAAAAAA) >>  1) | ((val & 0x55555555) <<  1);
    return val;
}

uint32_t Sp0256::Getb(int len)
{
    uint32_t data;

    if (m_fifo_sel)
    {
        uint32_t d0 = m_fifo[(m_fifo_tail    ) & 63];
        uint32_t d1 = m_fifo[(m_fifo_tail + 1) & 63];
        data = ((d1 << 10) | d0) >> m_fifo_bitp;

        m_fifo_bitp += len;
        if (m_fifo_bitp >= 10) { m_fifo_tail++; m_fifo_bitp -= 10; }
    }
    else
    {
        int idx0 = (m_pc    ) >> 3;
        int idx1 = (m_pc + 8) >> 3;
        uint32_t d0 = m_rom[idx0 & 0xffff];
        uint32_t d1 = m_rom[idx1 & 0xffff];
        data = ((d1 << 8) | d0) >> (m_pc & 7);
        m_pc += len;
    }

    data &= (uint32_t(1) << len) - 1;
    return data;
}

void Sp0256::Micro()
{
    uint8_t immed4, opcode;
    uint16_t cr;
    int ctrl_xfer;
    int repeat;
    int i, idx0, idx1;

    // Safety fuse: well-formed allophone ROM data always resolves to a
    // repeat count or a halt within a handful of iterations. This bound
    // is not present in the original hardware/ROM (which is trusted) but
    // protects the host process if the user points the DLL at a
    // corrupt or wrong ROM file, so a bad file hangs speech instead of
    // hanging VCC.
    int safety = 4096;

    while (m_filt.rpt <= 0)
    {
        if (--safety <= 0) { m_halted = 1; m_pc = 0; m_filt.rpt = 1; SetSby(1); return; }

        if (m_halted && !m_lrq)
        {
            m_pc       = m_ald | (0x1000 << 3);
            m_fifo_sel = 0;
            m_halted   = 0;
            m_lrq      = 1;
            m_ald      = 0;
            for (i = 0; i < 16; i++) m_filt.r[i] = 0;
            if (DataRequestChanged) DataRequestChanged(true);
        }

        if (m_halted)
        {
            m_filt.rpt = 1;
            m_lrq      = 1;
            m_ald      = 0;
            for (i = 0; i < 16; i++) m_filt.r[i] = 0;
            SetSby(1);
            return;
        }

        immed4 = uint8_t(Getb(4));
        opcode = uint8_t(Getb(4));
        repeat = 0;
        ctrl_xfer = 0;

        switch (opcode)
        {
            case 0x0:
            {
                if (immed4)
                {
                    m_page = bitrev32(immed4) >> 13;
                }
                else
                {
                    uint32_t btrg = uint32_t(m_stack);
                    m_stack = 0;
                    if (!btrg) { m_halted = 1; m_pc = 0; ctrl_xfer = 1; }
                    else { m_pc = int(btrg); ctrl_xfer = 1; }
                }
                break;
            }

            case 0xE:
            case 0xD:
            {
                uint32_t btrg = m_page | (bitrev32(immed4) >> 17) | (bitrev32(Getb(8)) >> 21);
                ctrl_xfer = 1;
                if (opcode == 0xD) m_stack = (m_pc + 7) & ~7;
                m_pc = int(btrg);
                break;
            }

            case 0x1:
            {
                m_mode = uint32_t(((immed4 & 8) >> 2) | (immed4 & 4) | ((immed4 & 3) << 4));
                break;
            }

            default:
            {
                repeat = immed4 | (m_mode & 0x30);
                break;
            }
        }
        if (opcode != 1) m_mode &= 0xF;

        if (ctrl_xfer)
        {
            m_fifo_sel = (m_pc == kFifoAddr) ? 1 : 0;
            if (m_fifo_sel && m_fifo_bitp)
            {
                if (m_fifo_tail < m_fifo_head) m_fifo_tail++;
                m_fifo_bitp = 0;
            }
            continue;
        }

        if (!repeat) continue;

        m_filt.rpt = repeat + 1;

        i = (opcode << 3) | int(m_mode & 6);
        idx0 = sp0256_df_idx[i++];
        idx1 = sp0256_df_idx[i];

        for (i = idx0; i <= idx1; i++)
        {
            int len, shf, delta, field, prm, clra, clr5;
            int8_t value;

            cr = sp0256_datafmt[i];
            len   = CR_LEN(cr);
            shf   = CR_SHF(cr);
            prm   = CR_PRM(cr);
            clra  = cr & CR_CLRA;
            clr5  = cr & CR_CLR5;
            delta = cr & CR_DELTA;
            field = cr & CR_FIELD;
            value = 0;

            if (clra)
            {
                for (int j = 0; j < 16; j++) m_filt.r[j] = 0;
                m_silent = 1;
            }
            if (clr5) m_filt.r[B5] = m_filt.r[F5] = 0;

            if (len)
            {
                value = int8_t(Getb(len));
            }
            else
            {
                continue;
            }

            if (delta)
            {
                if (value & (1 << (len - 1))) value = int8_t(unsigned(value) | (~0u << len));
            }
            if (shf) value = int8_t(value << shf);

            m_silent = 0;

            if (field)
            {
                m_filt.r[prm] &= uint8_t(~(~0u << shf));
                m_filt.r[prm] |= uint8_t(value);
                continue;
            }

            if (delta)
            {
                m_filt.r[prm] = uint8_t(m_filt.r[prm] + value);
                continue;
            }

            m_filt.r[prm] = uint8_t(value);
        }

        if (opcode == 0xF)
        {
            m_silent = 1;
            m_filt.r[1] = kPerPause;
        }

        m_filt.Regdec();
        break;
    }
}

void Sp0256::Ald(uint8_t data)
{
    if (!m_lrq) return; // busy, drop write (matches real chip behavior)

    m_lrq = 0;
    m_ald = int(data) << 4;
    if (DataRequestChanged) DataRequestChanged(false);
    SetSby(0);
}

void Sp0256::GenerateSamples(int16_t* out, int count)
{
    // The internal scratch ring buffer is kScbufSize samples. A single
    // request for >= that many samples can make the head pointer wrap
    // all the way back onto the tail pointer, which makes a full buffer
    // indistinguishable from an empty one and hangs the drain loop below
    // (this edge case exists in the original hardware-accurate algorithm
    // too, it's just never exercised there because callers only ever ask
    // for small audio-callback-sized chunks). Chunk any larger request
    // so that invariant is never violated.
    constexpr int kMaxChunk = kScbufSize / 2;
    while (count > kMaxChunk) {
        GenerateSamples(out, kMaxChunk);
        out += kMaxChunk;
        count -= kMaxChunk;
    }

    int output_index = 0;

    while (output_index < count)
    {
        while (m_sc_tail != m_sc_head)
        {
            out[output_index++] = m_scratch[m_sc_tail++ & kScbufMask];
            // m_sc_tail must be wrapped back into the ring buffer's index
            // range immediately, exactly like MAME's own drain loop does
            // (`m_sc_tail &= SCBUF_MASK;` right here) -- m_sc_head is
            // masked after every write in the block below, so leaving
            // m_sc_tail unmasked lets it grow past m_sc_head's wrapped
            // value the first time total output crosses a kScbufSize
            // (4096-sample) boundary. From that point on `m_sc_tail !=
            // m_sc_head` is permanently (falsely) true -- this loop keeps
            // "draining" stale/stale-wrapped scratch contents forever and
            // Micro()/Update() are never invoked again, silently freezing
            // all further speech a fraction of a second after the chip
            // starts ticking (this was the root cause of "cartridge loads
            // and commands reach the firmware, but no sound ever plays").
            m_sc_tail &= kScbufMask;
            if (output_index >= count) break;
        }
        if (output_index >= count) break;

        int length = count - output_index;
        int did_samp = 0;
        if (length > 0) do
        {
            if (m_filt.rpt <= 0) Micro();

            int do_samp = length - did_samp;
            if (int(m_sc_head + do_samp - m_sc_tail) > kScbufSize)
                do_samp = int(m_sc_tail + kScbufSize - m_sc_head);

            if (do_samp == 0) break;

            if (m_silent && m_filt.rpt <= 0)
            {
                uint32_t y = m_sc_head;
                for (int x = 0; x < do_samp; x++) m_scratch[y++ & kScbufMask] = 0;
                m_sc_head += do_samp;
                did_samp += do_samp;
            }
            else
            {
                did_samp += m_filt.Update(do_samp, m_scratch.get(), &m_sc_head);
            }
            m_sc_head &= kScbufMask;

        } while (m_filt.rpt >= 0 && length > did_samp);
    }
}

} // namespace ssc
