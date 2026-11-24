// psg_input.h
//
// Host input for the CoCo PSG gameports (Windows): game controllers via
// the WinMM joystick API, or the numeric keypad.
//
// Gamepad mapping (XInput pads appear to WinMM with these numbers):
//   stick / d-pad -> directions
//   button 1 (A) -> A / fire     button 2 (B) -> B      button 3 (X) -> C
//   button 4 (Y) -> X            button 5 (LB) -> Y     button 6 (RB) -> Z
//   button 7 (Back) -> Mode      button 8 (Start) -> Start
// Numeric keypad (NumLock on; only while VCC has focus):
//   8 2 4 6 (and 7 9 1 3 diagonals) -> directions
//   0 -> A / fire   . -> B   + -> C   - -> Start

#pragma once
#include <cstdint>
#include <string>
#include "psg_cart.h"

namespace psg {

struct InputSource {
    enum Kind : uint8_t { None, Numpad, Joystick } kind = None;
    int joy = 0;    // WinMM joystick id (0-15) when kind == Joystick

    std::string ToConfig() const;                    // "none", "numpad", "joy1"...
    static InputSource FromConfig(const std::string& s);
    std::string Label() const;                       // for the menu
    InputSource Next() const;                        // cycle through what's available
};

// Current buttons (PadButton bits) from a source.
uint16_t PollInput(const InputSource& src);

std::string PadTypeConfig(PadType t);                // "atari", "genesis3", "genesis6"
PadType PadTypeFromConfig(const std::string& s);
std::string PadTypeLabel(PadType t);
PadType NextPadType(PadType t);

} // namespace psg
