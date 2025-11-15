#pragma once
#include "vcc/detail/exports.h"
#include <iostream>



namespace vcc::utils
{

	LIBCOMMON_EXPORT [[nodiscard]] std::iostream::pos_type get_stream_size(std::iostream& stream);

}
