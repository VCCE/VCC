// sst39sf040.cpp -- see sst39sf040.h.

#include "sst39sf040.h"
#include <algorithm>
#include <cstring>

namespace psg {

void Sst39sf040::Load(const uint8_t* data, size_t len)
{
    std::fill(m_mem.begin(), m_mem.end(), uint8_t(0xFF));
    len = std::min(len, kSize);
    if (data && len)
        std::memcpy(m_mem.data(), data, len);
    ResetCommandState();
}

void Sst39sf040::Erase()
{
    std::fill(m_mem.begin(), m_mem.end(), uint8_t(0xFF));
    ResetCommandState();
}

uint8_t Sst39sf040::Read(uint32_t addr) const
{
    addr &= kSize - 1;
    if (m_id_mode)
        return (addr & 1) ? kDeviceId : kMakerId;
    return m_mem[addr];
}

void Sst39sf040::Write(uint32_t addr, uint8_t data)
{
    addr &= kSize - 1;
    const uint32_t cmd = addr & 0x7FFF;

    switch (m_state) {
    case State::Read:
        if (cmd == 0x5555 && data == 0xAA)
            m_state = State::Unlock1;
        else if (data == 0xF0)
            m_id_mode = false;          // single-cycle ID exit
        return;

    case State::Unlock1:
        m_state = (cmd == 0x2AAA && data == 0x55) ? State::Unlock2 : State::Read;
        return;

    case State::Unlock2:
        m_state = State::Read;
        if (cmd != 0x5555)
            return;
        switch (data) {
        case 0xA0: m_state = State::Program; break;
        case 0x80: m_state = State::Erase; break;
        case 0x90: m_id_mode = true; break;
        case 0xF0: m_id_mode = false; break;
        default: break;
        }
        return;

    case State::Program:
        m_state = State::Read;
        if ((m_mem[addr] & data) != m_mem[addr]) {
            m_mem[addr] &= data;        // programming only clears bits
            m_changes++;
        }
        return;

    case State::Erase:
        m_state = (cmd == 0x5555 && data == 0xAA) ? State::Erase1 : State::Read;
        return;

    case State::Erase1:
        m_state = (cmd == 0x2AAA && data == 0x55) ? State::Erase2 : State::Read;
        return;

    case State::Erase2:
        m_state = State::Read;
        if (data == 0x10 && cmd == 0x5555) {
            std::fill(m_mem.begin(), m_mem.end(), uint8_t(0xFF));
            m_changes++;
        } else if (data == 0x30) {
            const auto base = m_mem.begin() + (addr & ~uint32_t(kSectorSize - 1));
            std::fill(base, base + kSectorSize, uint8_t(0xFF));
            m_changes++;
        }
        return;
    }
}

} // namespace psg
