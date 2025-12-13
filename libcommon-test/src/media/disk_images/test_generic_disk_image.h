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
#include "../memory_stream_buffer.h"
#include "vcc/media/disk_images/generic_disk_image.h"
#include "gtest/gtest.h"
#include <fstream>
#include <functional>


class test_generic_disk_image : public testing::Test
{
protected:

	using generic_disk_image = ::vcc::media::disk_images::generic_disk_image;
	using disk_error_id_type = generic_disk_image::error_id_type;
	using size_type = generic_disk_image::size_type;
	using geometry_type = generic_disk_image::geometry_type;
	using buffer_type = generic_disk_image::buffer_type;

	// TODO-CHET: Remove first_sector_id from parameter list
	using geometry_iterator_function_type = void(
		generic_disk_image& image,
		memory_stream_buffer&,
		const geometry_type&,
		size_type,
		size_type,
		size_type);


protected:

	static geometry_type create_geometry(
		size_type head_count = 2,
		size_type track_count = 8,
		size_type sector_count = 16,
		size_type sector_size = 16)
	{
		geometry_type geometry;

		geometry.head_count = head_count;
		geometry.track_count = track_count;
		geometry.sector_count = sector_count;
		geometry.sector_size = sector_size;

		return geometry;
	}

	static std::unique_ptr<generic_disk_image::stream_type> create_stream(
		memory_stream_buffer& stream_buffer,
		const geometry_type& geometry = test_geometry_)
	{
		const auto image_size(geometry.head_count * geometry.track_count * geometry.sector_count * geometry.sector_size);

		stream_buffer.resize(image_size);
		return std::make_unique<generic_disk_image::stream_type>(&stream_buffer);
	}

	void iterate_geometry(
		generic_disk_image& image,
		memory_stream_buffer& stream_buffer,
		const geometry_type& geometry,
		std::function<geometry_iterator_function_type> callback) const
	{
		for (auto head(0u); head < geometry.head_count; ++head)
		{
			for (auto track(0u); track < geometry.track_count; ++track)
			{
				for (auto sector(image.first_valid_sector_id());
					 sector < geometry.sector_count + image.first_valid_sector_id();
					 ++sector)
				{
					callback(image, stream_buffer, geometry, head, track, sector);
				}
			}
		}
	}

	static buffer_type::value_type make_validation_id(size_type head, size_type track, size_type sector)
	{
		return  static_cast<buffer_type::value_type>((head << 7) | (track << 4) | sector);
	}


	static size_type calculate_sector_position(
		const geometry_type& geometry,
		size_type first_sector_id,
		size_type head,
		size_type track,
		size_type sector)
	{
		const auto track_size(geometry.sector_count * geometry.sector_size);
		const auto image_offset(0);	//	FIXME: need this

		const auto head_offset(head * track_size * geometry.track_count);
		const auto track_offset(track * track_size);
		const auto sector_offset((sector - first_sector_id) * geometry.sector_size);

		return image_offset + head_offset + track_offset + sector_offset;
	}

	static void fill_sector(
		generic_disk_image& image,
		memory_stream_buffer& stream_buffer,
		const geometry_type& geometry,
		size_type head,
		size_type track,
		size_type sector)
	{
		const auto sector_position(calculate_sector_position(geometry, image.first_valid_sector_id(), head, track, sector));
		const auto id(make_validation_id(head, track, sector - image.first_valid_sector_id()));
		for (auto i(0u); i < geometry.sector_size; ++i)
		{
			stream_buffer[sector_position + i] = id;
		}
	}

	static void validate_sector_read(
		generic_disk_image& image,
		[[maybe_unused]] memory_stream_buffer& stream_buffer,
		const geometry_type& geometry,
		size_type head,
		size_type track,
		size_type sector)
	{
		buffer_type sector_buffer;

		const auto result(image.read_sector(head, track, head, track, sector, sector_buffer));
		ASSERT_EQ(result, decltype(result)::success);
		ASSERT_EQ(sector_buffer.size(), geometry.sector_size);

		const auto id(make_validation_id(head, track, sector - image.first_valid_sector_id()));
		for (const auto& data : sector_buffer)
		{
			ASSERT_EQ(data, id);
		}
	};

	static void validate_sector_write(
		generic_disk_image& image,
		[[maybe_unused]] memory_stream_buffer& stream_buffer,
		const geometry_type& geometry,
		size_type head,
		size_type track,
		size_type sector)
	{
		buffer_type sector_buffer(geometry.sector_size);

		const auto id(make_validation_id(head, track, sector - image.first_valid_sector_id()));
		for (auto& data : sector_buffer)
		{
			data = id;
		}

		ASSERT_EQ(image.write_sector(head, track, head, track, sector, sector_buffer), disk_error_id_type::success);
	};

	static void validate_sector_data(
		generic_disk_image& image,
		memory_stream_buffer& stream_buffer,
		const geometry_type& geometry,
		size_type head,
		size_type track,
		size_type sector)
	{
		const auto sector_position(calculate_sector_position(geometry, image.first_valid_sector_id(), head, track, sector));
		const auto id(make_validation_id(head, track, sector - image.first_valid_sector_id()));

		for (auto i(0u); i < geometry.sector_size; ++i)
		{
			ASSERT_EQ(stream_buffer[sector_position + i], id);
		}
	};

protected:

	static const inline geometry_type test_geometry_ = create_geometry();
	memory_stream_buffer buffer_stream_;
	static const inline auto test_value_sequence_ = { 1, 2, 3, 4, 5, 10 };
};
