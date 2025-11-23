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
#include <vector>
#include <strstream>


// FIXME: strstreambuf is deprecated but used here until a new solution that is
// lighter than the current alternative.
class memory_stream_buffer : public std::strstreambuf
{
public:
	using size_type = std::size_t;
	using buffer_type = std::vector<unsigned char>;
	using iterator = buffer_type::iterator;
	using const_iterator = buffer_type::const_iterator;
	using value_type = buffer_type::value_type;

	memory_stream_buffer() = default;

	void resize(size_type size)
	{
		buffer_.resize(size);

		setg(reinterpret_cast<char_type*>(buffer_.data()),
			 reinterpret_cast<char_type*>(buffer_.data()),
			 reinterpret_cast<char_type*>(buffer_.data() + buffer_.size()));

		setp(reinterpret_cast<char_type*>(buffer_.data()),
			 reinterpret_cast<char_type*>(buffer_.data()),
			 reinterpret_cast<char_type*>(buffer_.data() + buffer_.size()));
	}

	iterator begin()
	{
		return buffer_.begin();
	}

	iterator end()
	{
		return buffer_.end();
	}

	const_iterator begin() const
	{
		return buffer_.begin();
	}

	const_iterator end() const
	{
		return buffer_.end();
	}

	value_type& operator[](size_type index)
	{
		return buffer_[index];
	}

	value_type operator[](size_type index) const
	{
		return buffer_[index];
	}


private:

	buffer_type buffer_;
};
