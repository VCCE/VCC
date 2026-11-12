# CoCo PSG cartridge (psg.dll)

Emulation of Ed Snider's (Zippster) CoCo PSG: YM2149 sound chip, 512K FLASH,
512K SRAM, two joystick ports.
Docs: https://thezippsterzone.com/2018/05/08/coco-psg/

## Files

| File | Purpose |
|---|---|
| `ym2149.*`     | YM2149 core: tone, noise, 32-step envelope, I/O ports |
| `psg_input.*` | Host input for the gameports (gamepads, keypad) |
| `sst39sf040.*` | FLASH chip: command state machine, program/erase, software ID |
| `bandlimiter.*` | Band-limited resampler from the chip rate to the host rate |
| `psg_cart.*`   | Cartridge: `$FF5A-$FF5F` registers, banking, FLASH/SRAM, audio output |
| `psg_dll.cpp`  | VCC plugin exports, menu, ini setting |
| `test/psgharness.cpp` | Loads `psg.dll` without VCC and checks it (41 checks) |
| `test/padtest.cpp` | Gameport wiring checks against the core (Atari, Genesis 3/6) |
| `test/dis6809.py` | Small 6809 disassembler used to read the firmware |

## Status

Working:
- YM2149 at 2MHz, or 1MHz when control bit 0 is set
- Bank registers; FLASH (segments 0-63) and SRAM (128-191) in
  `$C000-$DFFF` / `$E000-$FEFF`
- SRAM writes with control bit 3 set (phase 2)
- SST39SF040 FLASH commands: byte program, 4K sector erase, chip erase,
  software ID ($BF/$B7). Command addresses via the bank windows, or via
  `$BFFF`/`$BAAA` with bits 3 and 5 set (phase 2)
- FLASH image picked from the cart menu (*CoCo PSG > Select Flash Image*),
  stored in the ini as `[CoCo-PSG] FlashImage=`. Programming/erasing is
  written back to that file 0.5s after the last change (and on unload); the
  first write-back keeps the original as `<image>.bak`. SRAM is not saved.
- Autostart: CART is asserted while an image is loaded and control bit 4 is 0
- Reset button (F5) keeps bank/control registers; hard reset (Shift+F5)
  clears them
- Mono output into the cart sound line (all three channels mixed, as on Rev 04)
- Gameports (phase 3) on YM ports A/B (R14/R15). Per port, from the cart menu
  (*CoCo PSG > Port A/B input* and *pad*; each click cycles, saved in the ini):
  - input: None, Numeric keypad, or any connected gamepad (WinMM)
  - pad: Atari stick, Genesis 3-button or Genesis 6-button (SEL = control
    bit 1 for port A, bit 2 for port B; 6-button sequence with 1.5ms reset)
  - gamepad buttons: 1 A/fire, 2 B, 3 C, 4 X, 5 Y, 6 Z, 7 Mode, 8 Start
    (Xbox pads: A B X Y LB RB Back Start); stick and d-pad for directions
  - keypad (NumLock on, VCC focused): 8 2 4 6 (+ diagonals), 0 fire/A,
    . B, + C, - Start. These keys may also reach the emulated CoCo keyboard.

VCC changes this needs (both optional exports; other carts are unaffected):
- `PakPowerOnReset`: called on hard reset instead of `PakReset`.
- `PakWriteMemoryByte(address, value)`: CPU writes to `$8000-$FEFF` while
  in ROM mode (previously dropped in `tcc1014mmu.cpp`) are offered to every
  cart with the full CPU address.

Not yet:
- Registers answering without `*SCS` when control bit 3 is set: only the
  MPI's SCS slot gets `$FF40-$FF5F`.
- Cart reads with control bit 3 set return normal data; on the real cart
  in an MPI they returned `$DF` (the firmware clears bit 3 before reading).

Audio path:
- The chip runs at its real internal rate (250kHz; 125kHz in 1MHz mode) into
  a buffer. Each sample VCC asks for (`PakSampleAudio`, 44.1kHz) is taken
  from that with a windowed-sinc low-pass (`bandlimiter.*`), so square-wave
  harmonics above 22kHz are removed instead of aliasing. An E7 square wave's
  worst inharmonic component is -63dB (it was -14dB when the chip was
  averaged once per scanline).
- Emulated time comes from the scanline count; the host sample rate is
  measured from the sample calls (shown in the status line). Register
  writes take effect with one-sample (23us) resolution.
- Measured YM2149 DAC levels (32 steps, as used by the ayumi emulator).
- Output is still 8-bit (VCC's cart audio format), TPDF-dithered; a channel
  at full volume swings about +/-32.
- CPU: about 25ms per emulated second in the Debug build (the two audio
  files are compiled optimised even in Debug).

## Measured on a real cart (Rev 04, CoCo 3 + MPI, 2026-11-07)

`PSGDUMP.BAS` probe results, now emulated:
- `$FF5A`, `$FF5B`, `$FF5D` read back what was written.
- `$FF5C` reads the control register (emulated as a mirror of `$FF5D`).
- `$FF5E` reads `$FF`.
- YM2149 registers read back all 8 bits (R1 written `$FF` reads `$FF`).
- I/O ports configured as inputs read `$FF` with nothing plugged in.
- The bank/control registers survive the reset button. The firmware menu
  depends on it: options 1-3 map segment 2/3/4 into bank 0, set autostart off
  and ask for RESET, and the CoCo 3 then boots HDB-DOS from `$C000`. VCC now
  tells carts a hard reset apart from a soft one (optional `PakPowerOnReset`
  export, called on hard reset only), so the PSG clears its registers on F9
  but keeps them on F5.

FLASH contents (`test/out/psgflash.rom`, dumped from the cart; contains Tandy
Disk BASIC/HDB-DOS code, so keep it out of the repo):
- seg 0: boot menu; seg 1: test firmware (copied to `$2000`); seg 2-4:
  HDB-DOS 1.5 for DriveWire 3 (CC1/CC2/CC3); seg 5-63: all `$00`.

Still unknown:
- Segments 64-127 / 192-255: the probe read them with control bit 3 set, and
  with bit 3 set every cart read in the MPI returned `$DF` (probably the
  cart driving *SLENB on reads too; the same probe's SRAM write-and-read-back
  is unreliable for the same reason). Emulated as mirrors; reads with bit 3
  set are emulated as normal reads.
- Whether the reset button resets the YM2149 (emulated: yes, it is silenced).
- Fixed volume 0 is treated as fully silent; the volume curve (1.5dB per DAC
  step) is nominal, not measured.

## Testing in VCC

Test files are in `test/`:

| File | What it checks |
|---|---|
| `out/psgtest.rom` | Test Flash image (built by `mkflash.py`). Segment 0 is `autostart.asm`: plays 440Hz on channel A, sets control bit 4, cold-starts BASIC. Every other byte of segment N is N |
| `tones.bas`  | Chords on all three channels, then the 1MHz clock bit (octave down) |
| `banks.bas`  | Maps segments with `$FF5A`/`$FF5B` and PEEKs `$C000`/`$E000` in ROM mode |
| `regs.bas`   | Register read-back (the hardware questions above) |
| `out/PSGDUMP.DSK` | `PSGDUMP.BAS` (from `psgdump.asm` via `mkdumpbas.py`, `mkdumpdsk.py`): on a real CoCo 3 + MPI + SDC, prints the register/mirror probe, then SAVEMs each non-blank FLASH segment as `PSGnn.BIN` (16 fit per disk; `out/PSGDMP2.DSK` is a blank spare) |
| `dsk2flash.py` | `python dsk2flash.py psgflash.rom PSGDUMP.DSK [PSGDMP2.DSK]` rebuilds the 512K image |

Keep control bit 4 set (`POKE &HFF5D,16`) while a Flash image is loaded:
writing `$FF5D` with bit 4 clear re-asserts CART, and BASIC then autostarts the
cart at `$C000`.

`regs.bas` expected in the emulator: `5 130 17`, `1 15`, `255`.

## Building and testing

```
MSBuild Vcc.sln /t:psg /p:Configuration=Debug /p:Platform=x86
```

Harness (from a VS x86 developer prompt):

```
cl /EHsc /O2 /std:c++17 /I ..\..\libcommon psgharness.cpp /Fe:psgharness.exe user32.lib
psgharness ..\..\__bin\Win32\Debug\psg.dll <out-dir>
```

It writes `psg_demo.wav` to `<out-dir>` for listening.
