#pragma once
#include <vcc/core/detail/exports.h>


namespace vcc::devices::rtc
{

	class LIBCOMMON_EXPORT cloud9
	{
	public:

		void set_read_only(bool value);

		unsigned char read_port(unsigned short port_id) const;
	};

}
