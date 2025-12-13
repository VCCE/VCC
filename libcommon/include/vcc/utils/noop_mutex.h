#pragma once
#include "vcc/detail/exports.h"
#include <memory>


namespace vcc::utils
{

	/// @brief A mutex with no actual locking mechanism.
	///
	/// This mutex has no actual locking mechanism and is used as a placeholder in when
	/// locking is not desirable such as a template type argument.
	struct noop_mutex
	{
		/// @brief Locks the mutex.
		constexpr void lock() const noexcept {}

		/// @brief Unlocks the mutex.
		constexpr void unlock() const noexcept {}

		/// @brief Tries to lock the mutex.
		/// 
		/// This function always succeeds.
		/// 
		/// @return `true`.
		constexpr bool try_lock() const noexcept { return true;  }


		/// @brief A shared instance for public use.
		LIBCOMMON_EXPORT static noop_mutex shared_instance_;
	};

}
