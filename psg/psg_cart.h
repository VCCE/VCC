// psg_cart.h
//
// CoCo PSG cartridge (Ed Snider / Zippster, 2018) -- portable core.
//
// From the CoCo PSG User's Manual:
//   $FF5A  memory bank 0 register   ($C000-$DFFF)
//   $FF5B  memory bank 1 register   ($E000-$FEFF)
//   $FF5D  control register
//            bit 0  YM2149 master clock, 0 = 2MHz, 1 = 1MHz
//            bit 1  gameport A SEL
//            bit 2  gameport B SEL
//            bit 3  write enable for FLASH/SRAM
//            bit 4  autostart, 0 = enabled, 1 = disabled
//            bit 5  FLASH programming enable
//   $FF5E  YM2149 register select
//   $FF5F  YM2149 data
//   All registers are zero at start-up.
//
// Measured on a real cart (Rev 04, CoCo 3 + MPI):
//   - $FF5A, $FF5B, $FF5D read back what was written.
//   - $FF5C reads the control register (treated as a mirror of $FF5D).
//   - $FF5E reads $FF (write only).
//   - The bank and control registers survive the reset button: the
//     firmware menu maps a HDB-DOS segment, sets autostart off and asks
//     for RESET, so only power-on clears them.
//
// Memory: 512K FLASH (SST39SF040) = segments 0-63, 512K SRAM = segments
// 128-191, each 8K. Any segment can be mapped into either bank.
//
// Writes (manual, "Using the Extended RAM" / "Programming the FLASH Chip"):
// with control bit 3 set the cart takes CPU writes to $C000-$FEFF and
// passes them to whichever chip the bank maps there. With bits 3 and 5
// set, writes to $BFFF and $BAAA reach the FLASH as its command addresses
// $5555 and $2AAA. VCC delivers these via PakWriteMemoryByte (ROM mode
// only; the manual requires RAM/ROM mode for both).

#pragma once
#include <cstdint>
#include <vector>
#include "bandlimiter.h"
#include "sst39sf040.h"
#include "ym2149.h"

namespace psg {

// Controller buttons, as a bitmask (1 = pressed).
enum PadButton : uint16_t {
    PadUp = 1, PadDown = 2, PadLeft = 4, PadRight = 8,
    PadA = 16, PadB = 32, PadC = 64, PadStart = 128,
    PadX = 256, PadY = 512, PadZ = 1024, PadMode = 2048,
};

// What is plugged into a gameport.
//   Atari:    pins 1-4 = up/down/left/right, pin 6 = fire (A), pin 9 = B.
//   Genesis3: SEL (pin 7, control bit 1/2) high: U D L R, pin 6 = B, pin 9 = C;
//             SEL low: U D, pins 3-4 low, pin 6 = A, pin 9 = Start.
//   Genesis6: as Genesis3, plus the 6-button sequence: on the 3rd SEL low
//             pulse pins 1-4 read low, on the following high they carry
//             Z Y X Mode, on the 4th low they read high. The count resets
//             after ~1.5ms without a SEL change.
enum class PadType : uint8_t { Atari, Genesis3, Genesis6 };

class PsgCart {
public:
    static constexpr size_t kSegmentSize = 0x2000;
    static constexpr size_t kChipSize    = 0x80000;   // 512K each
    static constexpr double kMasterClock = 2000000.0; // 2MHz oscillator
    // CoCo 3 horizontal rate: the GIME line is 912 clocks of 14.31818MHz
    // (228 colour-burst cycles), 15699.8Hz. VCC's event timing uses the
    // same CLK_LINE; emulated time for the chip is counted in these lines.
    static constexpr double kScanlineHz  = 14318180.0 / 912.0;

    PsgCart();

    // Power-on: all registers zero, YM2149 reset.
    void PowerOn();
    // Reset button: the CPLD registers are kept; the YM2149 is reset
    // (assumed: silences the chip, as the reset line normally would).
    void ResetButton();

    // FLASH image (up to 512K; shorter images are padded with $FF).
    void LoadFlash(const uint8_t* data, size_t len);
    void ClearFlash();
    bool HasFlash() const { return m_flash_loaded; }
    const std::vector<uint8_t>& FlashData() const { return m_flash.Data(); }
    uint32_t FlashChangeCount() const { return m_flash.ChangeCount(); }
    bool FlashIdMode() const { return m_flash.InIdMode(); }

    // Cartridge ports. `port` is the low byte of $FFxx.
    bool OwnsPort(uint8_t port) const { return port >= 0x5A && port <= 0x5F; }
    void WritePort(uint8_t port, uint8_t data);
    uint8_t ReadPort(uint8_t port) const;

    // Cartridge ROM space read. `address` is the offset VCC passes to
    // PakReadMemoryByte (0 = $C000 in 16K mode); A13 picks the bank.
    uint8_t ReadMemory(uint16_t address) const;
    // CPU write in ROM space, full CPU address ($8000-$FEFF).
    void CpuWrite(uint16_t address, uint8_t data);

    // Whether the CART line should be asserted (autostart).
    bool CartLine() const { return m_flash_loaded && !(m_control & 0x10); }

    // Audio timing. Emulated time comes from scanlines (TickScanline, called
    // once per HSYNC); NextSample() is called by VCC at the host audio rate
    // at evenly spaced points in emulated time. The chip runs at its own
    // 250kHz tick rate and each sample is band-limited from that, so there
    // is no aliasing, and register writes land with one-sample resolution.
    void TickScanline();
    // Next output sample: unsigned 8 bits centred on $80.
    uint8_t NextSample();
    // Host sample rate as measured from NextSample() calls (0 = none yet).
    double MeasuredSampleRate() const { return m_rate_measured; }

    // Gameports (0 = A on YM port A / R14, 1 = B on port B / R15).
    // Pins are active low; YM port bits: 0-3 = pins 1-4, 4 = pin 6,
    // 5 = pin 9, 6-7 not connected (read 1).
    void SetPad(int port, PadType type, uint16_t buttons);
    uint8_t PortPins(int port) const;

    uint8_t Bank0() const { return m_bank[0]; }
    uint8_t Bank1() const { return m_bank[1]; }
    uint8_t Control() const { return m_control; }
    Ym2149& Ym() { return m_ym; }
    const Ym2149& Ym() const { return m_ym; }

private:
    Ym2149 m_ym;
    Sst39sf040 m_flash;
    std::vector<uint8_t> m_sram;
    bool m_flash_loaded = false;

    uint8_t m_bank[2] {};
    uint8_t m_control = 0;

    void ResetAudio();
    void UpdatePortInputs();
    bool Sel(int port) const { return (m_control >> (port + 1)) & 1; }

    struct Pad {
        PadType type = PadType::Atari;
        uint16_t buttons = 0;
        int falls = 0;          // SEL high->low edges since the last timeout
        int idle_lines = 0;     // scanlines since the last SEL change
    };
    Pad m_pad[2];
    void ClockChip();               // one 250kHz step (chip ticks at 125kHz in 1MHz mode)
    void UpdateRate(double rate);

    BandLimiter m_bl;
    bool m_half_phase = false;      // 1MHz mode: chip ticks every other step
    double m_emu_ticks = 0.0;       // emulated time in 250kHz ticks
    double m_t_out = 0.0;           // time of the next output sample, in ticks
    double m_ticks_per_sample = 0.0;
    double m_rate_measured = 0.0;
    int m_win_lines = 0;
    int m_win_samples = 0;

    // DC-blocking filter state (the line-out is AC coupled) and dither.
    double m_dc_pole = 0.9996;
    double m_dc_x = 0.0;
    double m_dc_y = 0.0;
    uint32_t m_rng = 0x12345678;
};

} // namespace psg
