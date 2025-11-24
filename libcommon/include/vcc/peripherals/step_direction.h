#pragma once


namespace vcc::peripherals
{

	/// @brief Represents the directions a disk drive head can move.
	enum class step_direction
	{
		/// @brief Moves the disk drive head towards the inner most area of the disk.
		in,
		/// @brief Moves the disk drive head towards the outer most area of the disk.
		out
	};

}
