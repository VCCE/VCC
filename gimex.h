#pragma once
// gimex.h -- GIME-X extended GIME emulation.
//
// This file is part of VCC (Virtual Color Computer). VCC is free software:
// you can redistribute it and/or modify it under the terms of the GNU General
// Public License as published by the Free Software Foundation, either version 3
// of the License, or (at your option) any later version.

//
// GIME-X: FPGA replacement for the CoCo 3 GIME (Zippster hardware, Gary Becker Verilog).
//
// Extensions emulated here (see "Extended CoCo3 features in GIME-X"):
//   $FFE0        256-colour palette: copies palette 0 (9 bit) to entry <data>
//   $FFE1-$FFE3  DMA destination address (21 bits)
//   $FFE4-$FFE6  DMA source address (21 bits)
//   $FFE7        DMA control: bit 0 = write auto-increment, bit 1 = read auto-increment
//   $FFE8/$FFE9  DMA data (8 bit, or high/low of a 16 bit access)
//   $FFEF        firmware version: bits 7-6 major, bits 5-0 minor
//   Turbo speed  ($A5 -> $FFD9 or $FF96 bit 0 while in high speed) = 28.63636 MHz / 10
//

// enable / query GIME-X emulation (set from config; takes effect on hard reset)
void SetGimeXEnabled(bool enabled);
bool GimeXEnabled();

// reset GIME-X register state (hard and soft reset)
void GimeXReset();

// true if port (low byte of $FFxx) is decoded by the GIME-X
bool GimeXOwnsPort(unsigned char port);

void GimeXWrite(unsigned char port, unsigned char data);
unsigned char GimeXRead(unsigned char port);
// side-effect free read for the debugger / memory window
unsigned char GimeXSafeRead(unsigned char port);

// GIME-X turbo speed selection ($FFD9 = $A5, or $FF96 bit 0)
void SetGimeXTurbo(bool turbo);
