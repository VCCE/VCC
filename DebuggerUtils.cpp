#include "DebuggerUtils.h"
#include <sstream>
#include <iomanip>

std::string ToHexString(long value, int length, bool leadingZeros)
{
	std::ostringstream fmt;

	fmt << std::hex << std::setw(length) << std::setfill(leadingZeros ? '0' : ' ') << value;

	return fmt.str();
}


std::string ToDecimalString(long value, int length, bool leadingZeros)
{
	std::ostringstream fmt;

	fmt << std::dec << std::setw(length) << std::setfill(leadingZeros ? '0' : ' ') << value;

	return fmt.str();
}
