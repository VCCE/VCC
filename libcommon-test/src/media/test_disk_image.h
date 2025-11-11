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
	void read_sector(
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
	void write_sector(
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
