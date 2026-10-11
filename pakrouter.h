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
#pragma once

#include <vcc/bus/cartridge.h>

// The PakRouter handles routing and control of the plugin binary interface.
// An array of pointers to installed plugin objects is maintained. The array
// contains five slots, 0 = boot slot, 1..4 are MPI slots.
// Correct active slot, cts, and scs information is essential for routing.

#include <vcc/bus/cartridge_loader.h>

#define NumCartSlots 5

namespace VCC::Core
{
using plugin_ptr = typename cartridge_loader_result::cartridge_ptr_type;
	class PakRouter
	{
	public:
		// Constructor
		PakRouter::PakRouter(std::array<plugin_ptr, NumCartSlots>& slots)
			: slots_(slots) {};

		// UI driven
		void reset();

		void set_slots(std::array<cartridge*, 5> slots) {};

		void set_active_slot(unsigned int active_slot);
		void menu_item_clicked(unsigned int menu_item);

		// Exported plugin cpu loop operations
		void horizontal_sync();
		void write_port(unsigned char port, unsigned char data);
		unsigned char read_port(unsigned char port);
		unsigned char read_memory_byte(unsigned short address);
		unsigned short sample_audio();

		// Render loop callbacks
		void plugin_status(char * txt, size_t len);

		// Plugin cpu loop callbacks
		void cart_write_memory(int slot, unsigned char val, unsigned short adr);
		unsigned char cart_read_memory(int slot,unsigned short adr);
		void cart_assert_line(int slot, bool state);  // see line_states_
		void cart_assert_interrupt(int slot, Interrupt intr, InterruptSource src);

		// Test for inactive MPI
		inline bool mpi_not_active() const
		{
			return active_slot_ == 0;
		}

	private:

		// Important:  The following need to be private because only
		// the router has the active cart, cts, and scs information to
		// determine correct cart routing.

		// Test if a disk port
		inline bool is_disk_port(int port)
		{
			return port >= 0x40 && port <= 0x5F;
		}

		// Slot start is normally called by the UI thread.
		// Caution: Start of MPI can cause a cascade that could create a loop.
		inline void slot_start(int slot)
		{
			if (auto& cart = slots_[slot])
        		cart->start();
		}

		// Get plugin status text
		inline void slot_get_status(int slot, char * buf, size_t len)
		{
			if (auto& cart = slots_[slot])
				cart->status(buf,len);
		}

		// slot reset
		inline void slot_reset(int slot)
		{
			if (auto& cart = slots_[slot])
        		cart->reset();
		}

		// slot hsync
		inline void slot_hsync(int slot)
		{
			if (auto& cart = slots_[slot])
        		cart->process_horizontal_sync();
		}

		// get audio sample from slot
		inline int slot_sample_audio(int slot)
		{
			if (auto& cart = slots_[slot])
				return cart->sample_audio();
			return 0;
		}

		// Read slot memory
		inline unsigned char slot_read_memory(int slot, unsigned short address)
		{
			if (auto& cart = slots_[slot])
				return cart->read_memory_byte(address);
			return 0;
		}

		// Write slot port
		inline void slot_write_port(int slot,unsigned char port, unsigned char value)
		{
			if (auto& cart = slots_[slot])
				cart->write_port(port, value);
		}

		// Read slot port
		inline unsigned char slot_read_port(int slot, unsigned char port)
		{
			if (auto& cart = slots_[slot])
				return cart->read_port(port);
			return 0;
		}


	private:
		// Helper to limit status output from cartridge
		static const int gMaxSlotStatus = 24;
		void slot_status_(int slot, char* out, size_t maxout);

	private:
		// Slots for cartridge objects;
		std::array<plugin_ptr, NumCartSlots>& slots_;

		// active_slot_ refers to the slot which exposes ROM to
		// the cpu. If non-zero a multipak is assumed to be present
		int active_slot_ = 0; // (CTS cart pin 32)

		// disk_slot_ refers to the slot that can interact with disk
		// ports and which slot controls line state (CART signal)
		int disk_slot_ = 0;   // (SCS cart pin 36)

		// Plugins can use the cart line to trigger an IRQ. They use a callback
		// to set the control line. The router stores the value in the line
		// state array and forwards it to gime code which detects any change.
		// Additionally anytime SCS is changed the router forwards the value
		// saved for the SCS slot.
		std::array<bool,5> line_states_{}; // (CART line state pin 8)
	};
}

