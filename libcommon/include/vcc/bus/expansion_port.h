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
#include "vcc/bus/cartridges/empty_cartridge.h"
#include "vcc/bus/expansion_port_bus.h"
#include "vcc/utils/borrowed_ptr.h"
#include "vcc/utils/cartridge_loader.h"
#include "vcc/utils/noop_mutex.h"
#include "vcc/detail/exports.h"
#include <memory>


namespace vcc::bus
{

	/// @brief Expansion port used for managing and using cartridges.
	///
	/// @todo add more details.
	template<
		class MutexType_ = ::vcc::utils::noop_mutex,
		template<typename... Args_> class LockType_ = std::scoped_lock>
	class expansion_port
	{
	public:

		/// @brief Type alias to lengths, 1 dimension sizes, and indexes.
		using size_type = std::size_t;
		/// @brief Type alias for variable length status strings.
		using status_type = std::string;
		/// @brief Type alias for a windows handle managed with a unique_ptr.
		using managed_handle_type = ::vcc::utils::cartridge_loader_result::handle_type;
		/// @brief Type alias for the cartridge that can be inserted into the slot.
		using cartridge_type = ::vcc::bus::cartridge;
		/// @brief Type alias for a managed cartridge pointer.
		using cartridge_ptr_type = std::shared_ptr<cartridge_type>;
		/// @brief Type alias for the cartridge driver the expansion port communicates with.
		using driver_type = ::vcc::bus::cartridge_driver;
		/// @brief Type alias for a managed cartridge driver pointer.
		using driver_ptr_type = ::vcc::utils::borrowed_ptr<driver_type*>;
		/// @brief Type alias for a name container.
		using name_type = cartridge_type::name_type;
		/// @brief Type alias for a collection of menu items.
		using menu_item_collection_type = cartridge_type::menu_item_collection_type;
		/// @brief Type alias for a digital audio sample.
		using sample_type = driver_type::sample_type;
		/// @copydoc cartridge_type::menu_item_id_type
		using menu_item_id_type = cartridge_type::menu_item_id_type;

		using mutex_type = typename MutexType_;
		using scoped_lock_type = typename LockType_<mutex_type>;
		using dual_scoped_lock_type = typename LockType_<mutex_type, mutex_type>;


	public:

		/// @brief Construct an expansion port.
		/// 
		/// Construct an expansion to a default empty state. When constructed the cartridge
		/// managed in `shared_placeholder_cartridge_` will be used.
		expansion_port(mutex_type& cartridge_mutex, mutex_type& driver_mutex)
			:
			cartridge_mutex_(cartridge_mutex),
			driver_mutex_(driver_mutex),
			cartridge_(::vcc::bus::cartridges::empty_cartridge::shared_placeholder_cartridge_),
			driver_(&cartridge_->driver())
		{ }

		virtual ~expansion_port() = default;

		virtual void insert(managed_handle_type handle, cartridge_ptr_type cartridge)
		{
			if (cartridge == nullptr)
			{
				throw std::invalid_argument("Cannot insert cartridge. The cartridge is null.");
			}

			// Lock both mutexes so the entire operation is atomic
			dual_scoped_lock_type lock(cartridge_mutex_, driver_mutex_);

			eject();

			handle_ = move(handle);
			cartridge_ = move(cartridge);
			driver_ = &cartridge_->driver();
		};

		void eject()
		{
			// Lock both mutexes so the entire operation is atomic
			dual_scoped_lock_type lock(cartridge_mutex_, driver_mutex_);

			cartridge_.reset();
			driver_ = nullptr;
			handle_.reset();

			cartridge_ = ::vcc::bus::cartridges::empty_cartridge::shared_placeholder_cartridge_;
			driver_ = &cartridge_->driver();
		}

		/// @brief Specifies if the expansion port is empty.
		/// 
		/// @return `true` is the expansion port is empty; `false` otherwise.
		[[nodiscard]] bool empty() const
		{
			scoped_lock_type lock(cartridge_mutex_);

			// FIXME-CHET-NOW: This is seems like it might break easily. The empty state here can
			// only be caused through the default ctor (or a copy from one that default ctored).
			// Review and see if this is an actual problem.
			return cartridge_ == ::vcc::bus::cartridges::empty_cartridge::shared_placeholder_cartridge_;
		}

		/// @copydoc cartridge_type::name
		[[nodiscard]] name_type name() const
		{
			scoped_lock_type lock(cartridge_mutex_);

			return cartridge_->name();
		}

		/// @copydoc cartridge_type::start
		void start() const
		{
			scoped_lock_type lock(cartridge_mutex_);

			cartridge_->start();
		}

		/// @copydoc cartridge_type::stop
		void stop() const
		{
			scoped_lock_type lock(cartridge_mutex_);

			cartridge_->stop();
		}

		/// @copydoc cartridge_type::status
		[[nodiscard]] status_type status() const
		{
			// FIXME-CHET: The status function is called from the emulation loop which causes
			// it to lock up while the UI is open and locked with a different mutex. 
			scoped_lock_type lock(cartridge_mutex_);

			return cartridge_->status();
		}

		/// @copydoc cartridge_type::get_menu_items
		[[nodiscard]] menu_item_collection_type get_menu_items() const
		{
			scoped_lock_type lock(cartridge_mutex_);

			return cartridge_->get_menu_items();
		}

		/// @copydoc cartridge_type::menu_item_clicked
		void menu_item_clicked(menu_item_id_type menu_item_id) const
		{
			scoped_lock_type lock(cartridge_mutex_);

			return cartridge_->menu_item_clicked(menu_item_id);
		}

		/// @copydoc driver_type::reset
		void reset() const
		{
			scoped_lock_type lock(driver_mutex_);

			driver_->reset();
		}

		/// @copydoc driver_type::update
		void update(float delta) const
		{
			scoped_lock_type lock(driver_mutex_);

			driver_->update(delta);
		}

		/// @copydoc driver_type::write_port
		void write_port(unsigned char port_id, unsigned char value) const
		{
			scoped_lock_type lock(driver_mutex_);

			driver_->write_port(port_id, value);
		}

		/// @copydoc driver_type::read_port
		[[nodiscard]] unsigned char read_port(unsigned char port_id) const
		{
			scoped_lock_type lock(driver_mutex_);

			return driver_->read_port(port_id);
		}

		/// @copydoc driver_type::read_memory_byte
		[[nodiscard]] unsigned char read_memory_byte(size_type memory_address) const
		{
			scoped_lock_type lock(driver_mutex_);

			return driver_->read_memory_byte(memory_address);
		}

		/// @copydoc driver_type::sample_audio
		[[nodiscard]] sample_type sample_audio() const
		{
			scoped_lock_type lock(driver_mutex_);

			return driver_->sample_audio();
		}


	protected:

		mutex_type& cartridge_mutex_;
		mutex_type& driver_mutex_;

	private:

		/// @brief A pointer to the currently inserted cartridge.
		cartridge_ptr_type cartridge_;
		/// @brief A handle to the module that owns the currently inserted cartridge.
		managed_handle_type handle_;
		/// @brief A pointer to the hardware driver owned by the cartridge.
		driver_ptr_type driver_;
	};

}
