////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute it and/or
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
#include <optional>


namespace vcc::cartridges::fd502
{

	/// @brief The FD502 Control Register.
	///
	/// The Control Register is an additional register provided by the FD502 and not the
	/// WD179x. This register provides access to additional circuitry in the FD502 used for
	/// drive and head selection, drive motor control, controlling interrupt and halt
	/// signals, data density, and write precompensation (for double density disks).
	/// 
	/// This class provides easy access to the flags and settings controlled through the
	/// Control Register.
	class fd502_control_register
	{
	public:

		/// @brief Type alias for drive identifiers.
		using drive_id_type = std::size_t;
		/// @brief Type alias for drive head identifiers.
		using head_id_type = std::size_t;
		/// @brief Type alias for register values.
		using register_value_type = unsigned char;


	public:

		/// @brief Construct the Control Register.
		fd502_control_register() noexcept = default;

		/// @brief Construct a copy of an existing Register File.
		/// 
		/// @param other_register_file The register file to copy from.
		fd502_control_register(const fd502_control_register& other_register_file) = default;

		/// @brief Sets the Control Register
		/// 
		/// Sets the Control Register and decodes its bit flags and other properties.
		/// 
		/// @param register_value The register value containing the control flags.
		fd502_control_register& operator=(register_value_type register_value) noexcept;

		/// @brief Determines if a drive has been selected.
		/// 
		/// @return `true` if a drive has been selected; `false` otherwise.
		bool has_drive_selection() const noexcept
		{
			return drive_.has_value();
		}

		/// @brief Retrieve the selected drive.
		/// 
		/// @return The selected drive id.
		drive_id_type selected_drive() const
		{
			return drive_.value();
		}

		/// @brief Retrieve the selected drive head.
		/// 
		/// @return The selected drive head.
		head_id_type head() const noexcept
		{
			return head_;
		}

		/// @brief Retrieve the state of the motor enable line.
		/// 
		/// @return `true` if the motor line is enabled (i.e. drive motors on); `false`
		/// otherwise.
		bool motor_enabled() const noexcept
		{
			return motor_enabled_;
		}

		/// @brief Retrieve the state of the interrupt enable flag.
		/// 
		/// @return `true` if the FDC is configured to generate interrupts; `false` otherwise.
		bool interupt_enabled() const noexcept
		{
			return interupt_enabled_;
		}

		/// @brief Retrieve the flag indicating if halt behavior is enabled.
		/// 
		/// @return `true` if halt is enabled; `false` otherwise.
		bool halt_enabled() const noexcept
		{
			return halt_enabled_;
		}


	private:

		/// @brief The currently selected drive.
		std::optional<drive_id_type> drive_;
		/// @brief The currently selected head.
		head_id_type head_ = 0;
		/// @brief Flag indicating if the motor line is enabled.
		bool motor_enabled_ = false;
		/// @brief Flag indicating if the FDC is configured to generate interrupts.
		bool interupt_enabled_ = false;
		/// @brief Flag indicating if the FDC is configured to halt the CPU.
		bool halt_enabled_ = false;
	};

}
