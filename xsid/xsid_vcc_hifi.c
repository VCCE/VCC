/*
 * XSID.dll for VCC - CoCo X-SID compatible cartridge interface
 * v0.2.0 high-fidelity TEST build (2026-09-04)
 *
 * External interface is unchanged from locked v0.1.0:
 *   CoCo SCS $FF40-$FF5F -> SID register $00-$1F
 *   no cartridge ROM, no emulator-only registers
 *
 * Internal changes:
 *   - 250 kHz internal rendering (one sample every four SID clocks)
 *   - box integration/downsampling to VCC's scanline audio rate
 *   - filter state updated at 250 kHz rather than ~15.7 kHz
 *   - higher-precision accumulation until final 8-bit VCC conversion
 *   - deterministic envelope counter reset on gate transitions
 *
 * GPL-3.0-or-later.
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned int size32;

#define SID_CLOCK 1000000u
#define VCC_AUDIO_RATE 15720u
#define INTERNAL_DIV 4u
#define PHASE_MASK 0x00ffffffu

#define CTRL_GATE  0x01
#define CTRL_SYNC  0x02
#define CTRL_RING  0x04
#define CTRL_TEST  0x08
#define CTRL_TRI   0x10
#define CTRL_SAW   0x20
#define CTRL_PULSE 0x40
#define CTRL_NOISE 0x80

typedef struct Voice {
    u32 phase;
    u32 prev_phase;
    u32 noise;
    u32 rate_counter;
    u8 env;
    u8 env_state;
    u8 prev_gate;
    u8 pad;
} Voice;

static u8 regs[32];
static Voice voice[3];
static u32 sid_cycle_fraction;
static u8 render_divider;
static s32 filt_lp;
static s32 filt_bp;
static s32 last_subsample_q8;
static s32 quant_error_q8;
static u8 last_osc3;
static u8 last_env3;

static const u16 rate_period[16] = {
    9, 32, 63, 95, 149, 220, 267, 313,
    392, 977, 1954, 3126, 3907, 11720, 19532, 31251
};

/* Q15 Chamberlin/SVF frequency coefficient indexed by cutoff >> 3.
   Internal sample rate is SID_CLOCK / INTERNAL_DIV = 250 kHz. */
static const u16 filter_fc_q15[256] = {
    25, 49, 77, 106, 135, 166, 196, 227,
    259, 291, 323, 355, 388, 420, 453, 486,
    520, 553, 587, 621, 655, 689, 723, 757,
    792, 826, 861, 896, 931, 966, 1001, 1036,
    1071, 1107, 1142, 1178, 1213, 1249, 1285, 1320,
    1356, 1392, 1428, 1465, 1501, 1537, 1573, 1610,
    1646, 1683, 1719, 1756, 1792, 1829, 1866, 1903,
    1940, 1977, 2014, 2051, 2088, 2125, 2162, 2199,
    2237, 2274, 2311, 2349, 2386, 2424, 2461, 2499,
    2537, 2574, 2612, 2650, 2688, 2725, 2763, 2801,
    2839, 2877, 2915, 2953, 2991, 3030, 3068, 3106,
    3144, 3182, 3221, 3259, 3297, 3336, 3374, 3413,
    3451, 3490, 3528, 3567, 3606, 3644, 3683, 3722,
    3760, 3799, 3838, 3877, 3916, 3954, 3993, 4032,
    4071, 4110, 4149, 4188, 4227, 4266, 4305, 4345,
    4384, 4423, 4462, 4501, 4541, 4580, 4619, 4659,
    4698, 4737, 4777, 4816, 4856, 4895, 4934, 4974,
    5014, 5053, 5093, 5132, 5172, 5211, 5251, 5291,
    5330, 5370, 5410, 5450, 5489, 5529, 5569, 5609,
    5649, 5689, 5729, 5768, 5808, 5848, 5888, 5928,
    5968, 6008, 6048, 6088, 6128, 6169, 6209, 6249,
    6289, 6329, 6369, 6409, 6450, 6490, 6530, 6570,
    6611, 6651, 6691, 6731, 6772, 6812, 6853, 6893,
    6933, 6974, 7014, 7055, 7095, 7135, 7176, 7216,
    7257, 7297, 7338, 7379, 7419, 7460, 7500, 7541,
    7581, 7622, 7663, 7703, 7744, 7785, 7825, 7866,
    7907, 7948, 7988, 8029, 8070, 8111, 8151, 8192,
    8233, 8274, 8315, 8356, 8397, 8437, 8478, 8519,
    8560, 8601, 8642, 8683, 8724, 8765, 8806, 8847,
    8888, 8929, 8970, 9011, 9052, 9093, 9134, 9175,
    9216, 9257, 9298, 9339, 9381, 9422, 9463, 9504,
    9545, 9586, 9627, 9669, 9710, 9751, 9792, 9834
};

/* Q15 damping indexed by SID resonance nibble. */
static const u16 filter_damp_q15[16] = {
    44237, 41615, 38994, 36372, 33751, 31130, 28508, 25887, 23265, 20644, 18022, 15401, 12780, 10158, 7537, 4915
};

static s32 clamp_s32(s32 x, s32 lo, s32 hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static void clear_state(void)
{
    u32 i;
    for (i = 0; i < 32; ++i) regs[i] = 0;
    for (i = 0; i < 3; ++i) {
        voice[i].phase = 0;
        voice[i].prev_phase = 0;
        voice[i].noise = 0x7ffff8u;
        voice[i].rate_counter = 0;
        voice[i].env = 0;
        voice[i].env_state = 0;
        voice[i].prev_gate = 0;
        voice[i].pad = 0;
    }
    sid_cycle_fraction = 0;
    render_divider = 0;
    filt_lp = 0;
    filt_bp = 0;
    last_subsample_q8 = 0;
    quant_error_q8 = 0;
    last_osc3 = 0;
    last_env3 = 0;
}

static u16 voice_freq(u32 v)
{
    u32 b = v * 7u;
    return (u16)((u16)regs[b] | ((u16)regs[b + 1u] << 8));
}

static u16 voice_pw(u32 v)
{
    u32 b = v * 7u;
    return (u16)(((u16)regs[b + 2u] | ((u16)regs[b + 3u] << 8)) & 0x0fffu);
}

static u8 voice_ctrl(u32 v) { return regs[v * 7u + 4u]; }
static u8 voice_ad(u32 v)   { return regs[v * 7u + 5u]; }
static u8 voice_sr(u32 v)   { return regs[v * 7u + 6u]; }

static u32 env_effective_period(u8 env, u16 base)
{
    u32 mult = 1;
    if (env < 0x5d) mult = 2;
    if (env < 0x36) mult = 4;
    if (env < 0x1a) mult = 8;
    if (env < 0x0e) mult = 16;
    if (env < 0x06) mult = 30;
    return ((u32)base) * mult;
}

static void clock_envelope(u32 v)
{
    Voice *p = &voice[v];
    u8 ad = voice_ad(v);
    u8 sr = voice_sr(v);
    u8 gate = (u8)(voice_ctrl(v) & CTRL_GATE);
    u8 sustain = (u8)((sr >> 4) * 17u);
    u16 period;

    if (gate != p->prev_gate) {
        p->rate_counter = 0;
        p->env_state = gate ? 1u : 0u;
        p->prev_gate = gate;
    }

    if (p->env_state == 1u) {
        period = rate_period[(ad >> 4) & 15u];
        if (++p->rate_counter >= period) {
            p->rate_counter = 0;
            if (p->env < 255u) ++p->env;
            else p->env_state = 2u;
        }
    } else if (p->env_state == 2u) {
        if (p->env > sustain) {
            period = rate_period[ad & 15u];
            if (++p->rate_counter >= env_effective_period(p->env, period)) {
                p->rate_counter = 0;
                --p->env;
            }
        }
    } else if (p->env > 0u) {
        period = rate_period[sr & 15u];
        if (++p->rate_counter >= env_effective_period(p->env, period)) {
            p->rate_counter = 0;
            --p->env;
        }
    }
}

static void clock_oscillators_once(void)
{
    u32 v;
    u8 msb_rise[3];

    for (v = 0; v < 3; ++v) {
        Voice *p = &voice[v];
        u8 ctrl = voice_ctrl(v);
        p->prev_phase = p->phase;
        if (ctrl & CTRL_TEST) {
            p->phase = 0;
            p->noise = 0x7ffff8u;
        } else {
            p->phase = (p->phase + (u32)voice_freq(v)) & PHASE_MASK;
            if (!(p->prev_phase & 0x080000u) && (p->phase & 0x080000u)) {
                u32 fb = ((p->noise >> 22) ^ (p->noise >> 17)) & 1u;
                p->noise = ((p->noise << 1) | fb) & 0x7fffffu;
                if (!p->noise) p->noise = 0x7ffff8u;
            }
        }
        msb_rise[v] = (u8)((!(p->prev_phase & 0x800000u) &&
                              (p->phase & 0x800000u)) ? 1u : 0u);
    }

    for (v = 0; v < 3; ++v) {
        u32 mod = (v + 2u) % 3u;
        if ((voice_ctrl(v) & CTRL_SYNC) && msb_rise[mod])
            voice[v].phase = 0;
    }

    for (v = 0; v < 3; ++v) clock_envelope(v);
}

static u16 noise12(u32 n)
{
    u16 x = 0;
    x |= (u16)(((n >> 22) & 1u) << 11);
    x |= (u16)(((n >> 20) & 1u) << 10);
    x |= (u16)(((n >> 16) & 1u) << 9);
    x |= (u16)(((n >> 13) & 1u) << 8);
    x |= (u16)(((n >> 11) & 1u) << 7);
    x |= (u16)(((n >> 7)  & 1u) << 6);
    x |= (u16)(((n >> 4)  & 1u) << 5);
    x |= (u16)(((n >> 2)  & 1u) << 4);
    x |= (u16)((n >> 8) & 0x0fu);
    return x & 0x0fffu;
}

static u16 waveform12(u32 v)
{
    Voice *p = &voice[v];
    u8 ctrl = voice_ctrl(v);
    u16 out = 0x0fffu;
    u8 any = 0;
    u16 tri, saw, pulse, noise;
    u32 mod = (v + 2u) % 3u;

    if (ctrl & CTRL_TEST) return 0;

    saw = (u16)((p->phase >> 12) & 0x0fffu);
    tri = (u16)((p->phase >> 11) & 0x0fffu);
    if (p->phase & 0x800000u) tri ^= 0x0fffu;
    if ((ctrl & CTRL_RING) && (voice[mod].phase & 0x800000u))
        tri ^= 0x0fffu;
    pulse = (u16)(((p->phase >> 12) & 0x0fffu) >= voice_pw(v)
                    ? 0x0fffu : 0u);
    noise = noise12(p->noise);

    if (ctrl & CTRL_TRI)   { out &= tri;   any = 1; }
    if (ctrl & CTRL_SAW)   { out &= saw;   any = 1; }
    if (ctrl & CTRL_PULSE) { out &= pulse; any = 1; }
    if (ctrl & CTRL_NOISE) { out &= noise; any = 1; }
    if (!any) out = 0;
    return out;
}

/* Return one internal 250 kHz sample in signed Q8 VCC-DAC units.
   +/-127.0 is represented as +/-32512. */
static s32 render_subsample_q8(void)
{
    s32 routed = 0;
    s32 direct = 0;
    s32 voice_out[3];
    u32 v;
    u8 route = regs[0x17] & 0x07u;
    u8 mode = regs[0x18] & 0x70u;
    u8 vol = regs[0x18] & 0x0fu;
    u16 cutoff = (u16)(((u16)regs[0x15] & 7u) |
                       ((u16)regs[0x16] << 3));
    u8 res = (u8)(regs[0x17] >> 4);

    for (v = 0; v < 3; ++v) {
        u16 w = waveform12(v);
        s32 centered = (s32)w - 2048;
        voice_out[v] = (centered * (s32)voice[v].env) >> 8;
    }

    last_osc3 = (u8)(waveform12(2) >> 4);
    last_env3 = voice[2].env;

    for (v = 0; v < 3; ++v) {
        if (route & (1u << v)) routed += voice_out[v];
        else if (!(v == 2u && (regs[0x18] & 0x80u)))
            direct += voice_out[v];
    }

    if (route) {
        s32 f = (s32)filter_fc_q15[cutoff >> 3];
        s32 damp = (s32)filter_damp_q15[res];
        s32 notch = routed - ((filt_bp * damp) >> 15);
        s32 hp;

        filt_lp += (f * filt_bp) >> 15;
        hp = notch - filt_lp;
        filt_bp += (f * hp) >> 15;

        filt_lp = clamp_s32(filt_lp, -32768, 32768);
        filt_bp = clamp_s32(filt_bp, -32768, 32768);
        hp = clamp_s32(hp, -32768, 32768);

        if (mode & 0x10u) direct += filt_lp;
        if (mode & 0x20u) direct += filt_bp;
        if (mode & 0x40u) direct += hp;
        if (!(mode & 0x70u)) direct += routed;
    } else {
        /* Avoid stale filter energy bursting when routing is later enabled. */
        filt_lp -= filt_lp >> 8;
        filt_bp -= filt_bp >> 8;
    }

    /* Preserve fractional precision until the final PakSampleAudio result. */
    direct = (direct * (s32)vol * 256) / (15 * 48);
    direct = clamp_s32(direct, -32512, 32512);
    return direct;
}

static u8 sid_read(u8 reg)
{
    reg &= 0x1fu;
    if (reg == 0x19u || reg == 0x1au) return 0xffu;
    if (reg == 0x1bu) return last_osc3;
    if (reg == 0x1cu) return last_env3;
    if (reg >= 0x1du) return 0xffu;
    return regs[reg];
}

static void sid_write(u8 reg, u8 value)
{
    reg &= 0x1fu;
    if (reg <= 0x18u) regs[reg] = value;
}

__declspec(dllexport) const char* PakGetName(void)
{
    return "CoCo X-SID HiFi";
}

__declspec(dllexport) const char* PakGetCatalogId(void)
{
    return "XSID-VCC-020";
}

__declspec(dllexport) const char* PakGetDescription(void)
{
    return "CoCo X-SID compatible MOS8580 cartridge emulation, oversampled test core";
}

__declspec(dllexport) void PakInitialize(size32 slot_id,
    const char* configuration_path, void* vcc_window, const void* callbacks)
{
    (void)slot_id; (void)configuration_path; (void)vcc_window; (void)callbacks;
    clear_state();
}

__declspec(dllexport) void PakTerminate(void)
{
}

__declspec(dllexport) void PakReset(void)
{
    clear_state();
}

__declspec(dllexport) void PakWritePort(u8 port, u8 data)
{
    if (port >= 0x40u && port <= 0x5fu)
        sid_write((u8)(port - 0x40u), data);
}

__declspec(dllexport) u8 PakReadPort(u8 port)
{
    if (port >= 0x40u && port <= 0x5fu)
        return sid_read((u8)(port - 0x40u));
    return 0xffu;
}

__declspec(dllexport) u8 PakReadMemoryByte(u16 address)
{
    (void)address;
    return 0xffu;
}

__declspec(dllexport) u16 PakSampleAudio(void)
{
    u32 cycles;
    s32 sum_q8 = 0;
    u32 count = 0;
    s32 average_q8;
    s32 shaped_q8;
    s32 quantized;
    u8 b;

    sid_cycle_fraction += SID_CLOCK;
    cycles = sid_cycle_fraction / VCC_AUDIO_RATE;
    sid_cycle_fraction -= cycles * VCC_AUDIO_RATE;

    while (cycles--) {
        clock_oscillators_once();
        if (++render_divider >= INTERNAL_DIV) {
            render_divider = 0;
            last_subsample_q8 = render_subsample_q8();
            sum_q8 += last_subsample_q8;
            ++count;
        }
    }

    if (count) average_q8 = sum_q8 / (s32)count;
    else average_q8 = last_subsample_q8;

    /* First-order error feedback keeps quiet envelopes from turning into
       strongly tonal 8-bit quantization steps. */
    shaped_q8 = average_q8 + quant_error_q8;
    quantized = (shaped_q8 >= 0)
        ? ((shaped_q8 + 128) >> 8)
        : -(((-shaped_q8) + 128) >> 8);
    quantized = clamp_s32(quantized, -127, 127);
    quant_error_q8 = shaped_q8 - (quantized * 256);
    quant_error_q8 = clamp_s32(quant_error_q8, -255, 255);

    b = (u8)(quantized + 128);
    return (u16)(((u16)b << 8) | b);
}
