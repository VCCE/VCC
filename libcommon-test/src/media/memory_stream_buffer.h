#pragma once
#include <vector>
#include <strstream>


// FIXME: strstreambuf is deprecated but used here until a new solution that is
// lighter than the current alternative.
class memory_stream_buffer : public std::strstreambuf
{
public:
	using size_type = ::std::size_t;
	using buffer_type = ::std::vector<unsigned char>;
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
