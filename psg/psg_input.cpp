// psg_input.cpp -- see psg_input.h.

#include "psg_input.h"
#include <windows.h>
#include <mmsystem.h>
#include <cstdlib>

#pragma comment(lib, "winmm.lib")

namespace psg {

namespace {

constexpr int kMaxJoy = 16;

bool JoyConnected(int id)
{
    JOYINFOEX info {};
    info.dwSize = sizeof info;
    info.dwFlags = JOY_RETURNALL;
    return joyGetPosEx(UINT(id), &info) == JOYERR_NOERROR;
}

std::string JoyName(int id)
{
    JOYCAPSA caps {};
    if (joyGetDevCapsA(UINT(id), &caps, sizeof caps) != JOYERR_NOERROR)
        return "";
    return caps.szPname;
}

bool VccHasFocus()
{
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

bool Down(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

uint16_t PollNumpad()
{
    if (!VccHasFocus())
        return 0;
    uint16_t b = 0;
    if (Down(VK_NUMPAD8) || Down(VK_NUMPAD7) || Down(VK_NUMPAD9)) b |= PadUp;
    if (Down(VK_NUMPAD2) || Down(VK_NUMPAD1) || Down(VK_NUMPAD3)) b |= PadDown;
    if (Down(VK_NUMPAD4) || Down(VK_NUMPAD7) || Down(VK_NUMPAD1)) b |= PadLeft;
    if (Down(VK_NUMPAD6) || Down(VK_NUMPAD9) || Down(VK_NUMPAD3)) b |= PadRight;
    if (Down(VK_NUMPAD0)) b |= PadA;
    if (Down(VK_DECIMAL)) b |= PadB;
    if (Down(VK_ADD)) b |= PadC;
    if (Down(VK_SUBTRACT)) b |= PadStart;
    return b;
}

uint16_t PollJoystick(int id)
{
    JOYINFOEX info {};
    info.dwSize = sizeof info;
    info.dwFlags = JOY_RETURNALL;
    if (joyGetPosEx(UINT(id), &info) != JOYERR_NOERROR)
        return 0;

    // Axis ranges (cached; WinMM caps calls are slow-ish).
    static JOYCAPSA caps[kMaxJoy];
    static bool have[kMaxJoy];
    if (!have[id]) {
        if (joyGetDevCapsA(UINT(id), &caps[id], sizeof caps[id]) != JOYERR_NOERROR)
            return 0;
        have[id] = true;
    }
    const JOYCAPSA& c = caps[id];

    uint16_t b = 0;
    auto norm = [](DWORD v, UINT lo, UINT hi) {
        return hi > lo ? (double(v) - lo) / double(hi - lo) * 2.0 - 1.0 : 0.0;
    };
    const double x = norm(info.dwXpos, c.wXmin, c.wXmax);
    const double y = norm(info.dwYpos, c.wYmin, c.wYmax);
    if (x < -0.5) b |= PadLeft;
    if (x > 0.5) b |= PadRight;
    if (y < -0.5) b |= PadUp;
    if (y > 0.5) b |= PadDown;

    if ((c.wCaps & JOYCAPS_HASPOV) && info.dwPOV != JOY_POVCENTERED && info.dwPOV <= 36000) {
        const DWORD p = info.dwPOV;                 // hundredths of a degree
        if (p >= 29250 || p <= 6750) b |= PadUp;
        if (p >= 2250 && p <= 15750) b |= PadRight;
        if (p >= 11250 && p <= 24750) b |= PadDown;
        if (p >= 20250 && p <= 33750) b |= PadLeft;
    }

    static const uint16_t kButtons[8] = { PadA, PadB, PadC, PadX, PadY, PadZ, PadMode, PadStart };
    for (int i = 0; i < 8; i++)
        if (info.dwButtons & (1u << i))
            b |= kButtons[i];
    return b;
}

} // namespace

std::string InputSource::ToConfig() const
{
    switch (kind) {
    case Numpad: return "numpad";
    case Joystick: return "joy" + std::to_string(joy + 1);
    default: return "none";
    }
}

InputSource InputSource::FromConfig(const std::string& s)
{
    InputSource src;
    if (s == "numpad") {
        src.kind = Numpad;
    } else if (s.rfind("joy", 0) == 0) {
        const int n = std::atoi(s.c_str() + 3);
        if (n >= 1 && n <= kMaxJoy) {
            src.kind = Joystick;
            src.joy = n - 1;
        }
    }
    return src;
}

std::string InputSource::Label() const
{
    switch (kind) {
    case Numpad: return "Numeric keypad";
    case Joystick: {
        std::string s = "Gamepad " + std::to_string(joy + 1);
        if (!JoyConnected(joy))
            return s + " (not connected)";
        const std::string name = JoyName(joy);
        return name.empty() ? s : s + " (" + name + ")";
    }
    default: return "None";
    }
}

InputSource InputSource::Next() const
{
    InputSource n;
    if (kind == None) {
        n.kind = Numpad;
        return n;
    }
    // From the keypad or a gamepad, go to the next connected gamepad.
    const int start = kind == Joystick ? joy + 1 : 0;
    for (int id = start; id < kMaxJoy; id++) {
        if (JoyConnected(id)) {
            n.kind = Joystick;
            n.joy = id;
            return n;
        }
    }
    return n;   // None
}

uint16_t PollInput(const InputSource& src)
{
    switch (src.kind) {
    case InputSource::Numpad: return PollNumpad();
    case InputSource::Joystick: return PollJoystick(src.joy);
    default: return 0;
    }
}

std::string PadTypeConfig(PadType t)
{
    switch (t) {
    case PadType::Genesis3: return "genesis3";
    case PadType::Genesis6: return "genesis6";
    default: return "atari";
    }
}

PadType PadTypeFromConfig(const std::string& s)
{
    if (s == "genesis3") return PadType::Genesis3;
    if (s == "genesis6") return PadType::Genesis6;
    return PadType::Atari;
}

std::string PadTypeLabel(PadType t)
{
    switch (t) {
    case PadType::Genesis3: return "Genesis 3-button";
    case PadType::Genesis6: return "Genesis 6-button";
    default: return "Atari stick";
    }
}

PadType NextPadType(PadType t)
{
    switch (t) {
    case PadType::Atari: return PadType::Genesis3;
    case PadType::Genesis3: return PadType::Genesis6;
    default: return PadType::Atari;
    }
}

} // namespace psg
