#pragma once
#include "vcc/detail/exports.h"
#include <memory>


namespace vcc::utils
{

	struct noop_mutex
	{
		constexpr void lock() const noexcept {}
		constexpr void unlock() const noexcept {}
		constexpr bool try_lock() const noexcept { return true;  }

		LIBCOMMON_EXPORT static noop_mutex shared_instance_;
	};

}
