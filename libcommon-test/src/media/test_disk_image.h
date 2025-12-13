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
#include "vcc/media/disk_image.h"
#include "gtest/gtest.h"
#include <fstream>
#include <functional>


class concrete_disk_image : public ::vcc::media::disk_image
{ 
public:

	using ::vcc::media::disk_image::disk_image;


	//[[nodiscard]] size_type get_sector_count(
	//	size_type disk_head,
	//	size_type disk_track) const override
	//{
	//	throw std::runtime_error("Cannot test abstract member function.");
	//}

	//[[nodiscard]] bool is_valid_track_sector(
	//	size_type disk_head,
	//	size_type disk_track,
	//	size_type disk_sector) const noexcept override
	//{
	//	return false;
	//}

	[[nodiscard]] bool is_valid_sector_record(
		size_type disk_head,
		size_type disk_track,
		size_type head_id,
		size_type track_id,
		size_type sector_id) const noexcept override
	{
		return false;
	}

	/// @inheritdoc
	[[nodiscard]] size_type get_sector_size(
		size_type disk_head,
		size_type disk_track,
		size_type head_id,
		size_type track_id,
		size_type sector_id) const override
	{
		throw std::runtime_error("Cannot test abstract member function.");
	}

	/// @inheritdoc
	[[nodiscard]] std::optional<sector_record_header_type> query_sector_header_by_index(
		size_type disk_head,
		size_type disk_track,
		size_type disk_sector) const override
	{
		throw std::runtime_error("Cannot test abstract member function.");
	}

	/// @inheritdoc
	[[nodiscard]] error_id_type read_sector(
		size_type disk_head,
		size_type disk_track,
		size_type head_id,
		size_type track_id,
		size_type sector_id,
		buffer_type& data_buffer) override
	{
		throw std::runtime_error("Cannot test abstract member function.");
	}

	/// @inheritdoc
	[[nodiscard]] error_id_type write_sector(
		size_type disk_head,
		size_type disk_track,
		size_type head_id,
		size_type track_id,
		size_type sector_id,
		const buffer_type& data_buffer) override
	{
		throw std::runtime_error("Cannot test abstract member function.");
	}

	/// @inheritdoc
	[[nodiscard]] sector_record_vector read_track(
		size_type disk_head,
		size_type disk_track) override
	{
		throw std::runtime_error("Cannot test abstract member function.");
	}

	/// @inheritdoc
	void write_track(
		size_type disk_head,
		size_type disk_track,
		const sector_record_vector& sectors) override
	{
		throw std::runtime_error("Cannot test abstract member function.");
	}

protected:

	size_type get_sector_count_unchecked(
		size_type disk_head,
		size_type disk_track) const noexcept override
	{
		return 0;
	}

};


class test_disk_image : public testing::Test
{
protected:

	using disk_image = concrete_disk_image;
	using size_type = disk_image::size_type;
	using geometry_type = disk_image::geometry_type;


protected:

	static const inline geometry_type test_geometry_ = { 2, 8 };
	static const inline auto test_value_sequence_ = { 1, 2, 3, 4, 5, 10 };
};
