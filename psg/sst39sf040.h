// sst39sf040.h
//
// Microchip/SST SST39SF040 512K x 8 FLASH, as used on the CoCo PSG.
// Command set from the datasheet (addresses are A14-A0, so only the low
// 15 address bits are compared):
//   Byte program:   AA@5555 55@2AAA A0@5555  data@addr  (can only clear bits)
//   Sector erase:   AA@5555 55@2AAA 80@5555 AA@5555 55@2AAA 30@SA (4K sector)
//   Chip erase:     AA@5555 55@2AAA 80@5555 AA@5555 55@2AAA 10@5555
//   Software ID:    AA@5555 55@2AAA 90@5555 -> reads: A0=0 $BF (maker),
//                   A0=1 $B7 (device)
//   ID exit:        F0@any, or AA@5555 55@2AAA F0@5555
// Program and erase complete instantly, so data# and toggle-bit polling
// see the finished result on the first read.

#pragma once
#include <cstdint>
#include <vector>

namespace psg {

class Sst39sf040 {
public:
    static constexpr size_t kSize = 0x80000;
    static constexpr size_t kSectorSize = 0x1000;
    static constexpr uint8_t kMakerId = 0xBF;
    static constexpr uint8_t kDeviceId = 0xB7;

    Sst39sf040() : m_mem(kSize, 0xFF) {}

    void Load(const uint8_t* data, size_t len);
    void Erase();                       // all $FF, no command needed
    const std::vector<uint8_t>& Data() const { return m_mem; }

    uint8_t Read(uint32_t addr) const;
    void Write(uint32_t addr, uint8_t data);

    // Abort any half-entered command sequence (e.g. on power-on).
    void ResetCommandState() { m_state = State::Read; m_id_mode = false; }

    bool InIdMode() const { return m_id_mode; }
    // Incremented by every program/erase that changed the contents.
    uint32_t ChangeCount() const { return m_changes; }

private:
    enum class State { Read, Unlock1, Unlock2, Program, Erase, Erase1, Erase2 };

    std::vector<uint8_t> m_mem;
    State m_state = State::Read;
    bool m_id_mode = false;
    uint32_t m_changes = 0;
};

} // namespace psg
