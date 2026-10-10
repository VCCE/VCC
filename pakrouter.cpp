//#define USE_LOGGING
////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute itand/or
//	modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your
//	option) any later version.
//	
//	VCC (Virtual Color Computer) is distributed in the hope that it will be
//	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
//	Public License for more details.
//	
//	You should have received a copy of the GNU General Public License along with
//	VCC (Virtual Color Computer). If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////

#include "mc6821.h"
#include "MachineDefs.h"
#include "tcc1014registers.h"
#include "tcc1014mmu.h"
#include "pakrouter.h"
#include <array>
#include <vcc/bus/cartridge.h>
#include <vcc/bus/cpak_cartridge.h>
#include <vcc/util/logger.h>
#include <typeinfo>
#include <algorithm>

//------------------------------------------------------------------------
// pakrouter handles routing and control of the plugin binary interface
// An array of pointers to installed plugin objects is used. The array
// contains five slots, 0 = boot slot, 1..4 are MPI slots, if present.
// binary calls are routed to/from either boot slot or the MPI slots.
// Correct active slot, cts, and scs information is essential for routing.
//------------------------------------------------------------------------

namespace VCC::Core
{
	// Set the active slot 0..4  Usually from MPI message.
	void PakRouter::set_active_slot(unsigned active_slot)
	{
		DLOG_C("PakRouter::set_active_slot %d\n",active_slot);
		disk_slot_ = active_slot;
		active_slot_ = active_slot;
		SetCart(line_states_[active_slot]);
	}

	// reset() is invoked on hard reset or power up.
	void PakRouter::reset()
	{
		DLOG_C("PakRouter::reset\n");
		for (int i = 0; i <= 4; i++)
			slot_reset(i);
	}

	// Process external menu item clicks
	void PakRouter::menu_item_clicked(unsigned int menu_item) {
		DLOG_C("PakRouter::menu_item_clicked %d %d %d\n",menu_item);
		// Each slot is allocated 50 menu items
		int item = menu_item % 50;
		int slot = menu_item / 50;
		if (auto& cart = slots_[slot])
			cart->menu_item_clicked(item);
	}

	//-------------------
	// Callbacks
	//-------------------

	// write and read memory are used by carts to do DMA (Hard disk carts do this)
	void PakRouter::cart_write_memory(int slot, unsigned char val, unsigned short adr)
	{
		MemWrite8(val, adr);
	};
	unsigned char PakRouter::cart_read_memory(int slot,unsigned short adr)
	{
		return MemRead8(adr);
	};

	// Assert line is the only callback that is slot sensitive
	void PakRouter::cart_assert_line(int slot, bool state)
	{
		if (slot == active_slot_) SetCart(state);
		line_states_[slot] = state;
	};

	// Cart Assert NMI is used by fd502
	void PakRouter::cart_assert_interrupt(int slot, Interrupt intr, InterruptSource src)
	{
		(void) src; // not used, might be used for state someday...

		//----------------------------------------------------------
		// Convert PAK interrupt assert to CPU assert or Gime assert.
		// FIXME: This is not correct. On a real Coco the CART line
		// is used to generate the INT. Both SCS and ACIA do this and
		// should be fixed.  Also IRQ and FIRQ should comtain state
		// so they can be edge triggered.
		//----------------------------------------------------------
		switch (intr) {
		case INT_CART:
			GimeAssertCartInterupt();  // Pass state so Gime can edge trigger?
			break;
		case INT_NMI:
			CPUAssertInterupt(IS_NMI, INT_NMI);
			break;
		}
	};

	//-------------------
	// Routed plugin calls
	//-------------------

	void PakRouter::horizontal_sync()
	{
		if (mpi_not_active()) {
			slot_hsync(0);
			return;
		}
		for (int i = 4; i > 0; i--)
			slot_hsync(i);
	}

	unsigned short PakRouter::sample_audio()
	{
		// Cartridge audio is two packed unsigned 8-bit channels. Audio-producing
		// cartridges use 0x80 as the midpoint level, while cartridges without
		// PakSampleAudio return 0 through the compatibility shim. The old code
		// added complete 16-bit packed samples, which allowed the right channel
		// to carry into the left channel and made two 0x8080 midpoint samples wrap to
		// 0x0100. Mix the channels independently around 0x80 instead.
		// ALTERNATE: Return after first cart that supplies a non-zero sample???
	
		int left = 0;
		int right = 0;
		int mask = 0xFF;
		int center = 0x80;
		bool have_sample = false;

		// if mpi is not loaded return sample from boot slot
		if (mpi_not_active()) {
			return slot_sample_audio(0);
		}
		// sum samples for MPI slots
		for (int i = 1; i <= 4; i++) {
			int sample = slot_sample_audio(i);
			if (sample != 0) {
				have_sample = true;
				left += ((sample >> 8) & mask) - center;
				right += (sample & mask) - center;
			}
		};
		if (!have_sample) return 0;
		left = std::clamp(left + center, 0, mask);
		right = std::clamp(right + center, 0, mask);
		return right + (left << 8);
	}

	unsigned char PakRouter::read_memory_byte(unsigned short address)
	{
		// Read only active slot memory
		return PakRouter::slot_read_memory(active_slot_,address);
	}

	void PakRouter::write_port(unsigned char port, unsigned char value)
	{
		// No mpi just write the port. For sure we don't want to
		// mess with the pakrouter's idea of what scs and cts are
		if (mpi_not_active()) {
			if (auto& cart = slots_[0])
				cart->write_port(port, value);
			return;
		}

		// Slot-select register (0x7F)
		if (port == 0x7F) {
			int scs = value & 3;
			int cts = (value >> 4) & 3;
			disk_slot_ = scs + 1;
			active_slot_ = cts + 1;
			return;
		}
		// Disk controller ports (0x40–0x5F) scs slot only
		if (is_disk_port(port)) {
			PakRouter::slot_write_port(disk_slot_, port, value);
			return;
		}
		// Broadcast other port writes
		for (int i = 1; i <= 4; i++) {
			PakRouter::slot_write_port(i, port, value);
		}
	}

	unsigned char PakRouter::read_port(unsigned char port)
	{
		// No mpi just return what ever the port says
		if (mpi_not_active()) {
			if (auto& cart = slots_[0])
				return cart->read_port(port);
			return 0;
		}

		// Slot-select register (0x7F)
		if (port == 0x7F) {
			int scs = (disk_slot_ -1) & 3;
			int cts = (active_slot_ -1) & 3;
			return (cts << 4) | scs;
		}
		// Disk controller ports (0x40–0x5F) scs slot only
		if (is_disk_port(port)) {
			return PakRouter::slot_read_port(disk_slot_, port);
		}
		// No MPI slot zero only
		if (mpi_not_active()) {
			return PakRouter::slot_read_port(0, port);
		}
		// MPI ports priority scan
		for (int i = 1; i <= 4; i++) {
			unsigned char data = PakRouter::slot_read_port(i, port);
			if (data != 0) return data;
		}
		return 0;
	}

	// Build combined status line for loaded carts
	void PakRouter::slot_status_(int slot, char* out, size_t maxout)
	{
		struct Buffer {
			char tmp[gMaxSlotStatus*8]; // Well oversized buffer
			uint32_t canary;
		};
		Buffer buf{};
		buf.canary = 0xdeadbeef;

		PakRouter::slot_get_status(slot, buf.tmp, gMaxSlotStatus);

		if (buf.canary != 0xdeadbeef) {
			DLOG_C("pakrouter.plugin_status overflow slot %d\n", slot);
			out[0] = '\0';
			return;
		}
		std::snprintf(out, maxout, "%.*s", maxout, buf.tmp);
	}
	void PakRouter::plugin_status(char* txt, size_t len)
	{
		txt[0] = '\0';

		// No MPI or no multislot active
		if (mpi_not_active()) {
			char tmp[gMaxSlotStatus+1]{};
			slot_status_(0, tmp, sizeof(tmp));
			if (tmp[0]) {
				std::snprintf(txt, len, "%s", tmp);
			}
			return;
		}
		// MPI header
		std::snprintf(txt, len, "MPI:%d,%d", active_slot_, disk_slot_);
		// Append each MPI slot
		for (unsigned int i = 1; i <= 4; i++) {
			char tmp[32]{};
			slot_status_(i, tmp, sizeof(tmp));
			if (!tmp[0]) continue;
			size_t used = std::strlen(txt);
			if (used >= len) break;
			std::snprintf(txt + used, len-used, " | %s", tmp);
		}
	}
}

