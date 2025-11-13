#include "test_generic_disk_image.h"
#include <vcc/media/exceptions.h>


// Validates the constructor throws an exception on invalid arguments.
TEST_F(test_generic_disk_image, invalid_ctor_arguments)
{
	// Must throw on a nullptr stream
	EXPECT_THROW(generic_disk_image({}, test_geometry_, 0, 0, false), std::invalid_argument);

	// Must throw if stream is an fstream and not open.
	EXPECT_THROW(
		generic_disk_image(std::make_unique<std::fstream>(), test_geometry_, 0, 0, false),
		std::invalid_argument);

#ifdef INCLUDE_INCOMPLETE_AND_BROKEN_LIBCOMMON_CODE
	// Must throw if stream size is too small
	auto stream(create_stream(buffer_stream_, test_geometry_));
	buffer_stream_.resize(1);

	EXPECT_THROW(
		generic_disk_image(move(stream), test_geometry_, 0, 0, true),
		std::runtime_error);
#endif
}

// Validates the head count returned is the same value passed to the constructor.
TEST_F(test_generic_disk_image, is_write_protected)
{
	EXPECT_TRUE(generic_disk_image(
		create_stream(buffer_stream_, test_geometry_),
		test_geometry_,
		0,
		0,
		true).is_write_protected());

	EXPECT_FALSE(generic_disk_image(
		create_stream(buffer_stream_, test_geometry_),
		test_geometry_,
		0,
		0,
		false).is_write_protected());
}


// Validates the head validation
TEST_F(test_generic_disk_image, is_valid_disk_head)
{
	geometry_type geometry;
	geometry.head_count = *std::max_element(test_value_sequence_.begin(), test_value_sequence_.end()) + 1;

	for (const auto head : test_value_sequence_)
	{
		EXPECT_TRUE(generic_disk_image(create_stream(buffer_stream_, geometry), geometry).is_valid_disk_head(head));
	}
}

// Validates the track validation
TEST_F(test_generic_disk_image, is_valid_disk_track)
{
	geometry_type geometry;
	geometry.track_count = *std::max_element(test_value_sequence_.begin(), test_value_sequence_.end()) + 1;

	for (const auto track : test_value_sequence_)
	{
		EXPECT_TRUE(generic_disk_image(create_stream(buffer_stream_, geometry), geometry).is_valid_disk_track(track));
	}
}

// Validates the head count returned is the same as the one specified in the geometry
TEST_F(test_generic_disk_image, head_count_property)
{
	for (const auto head_count : test_value_sequence_)
	{
		geometry_type geometry;
		geometry.head_count = head_count;

		EXPECT_EQ(
			generic_disk_image(create_stream(buffer_stream_, geometry), geometry).head_count(),
			geometry.head_count);
	}
}

// Validates the track count returned is the same as the one specified in the geometry
TEST_F(test_generic_disk_image, track_count_property)
{
	for (const auto track_count : test_value_sequence_)
	{
		geometry_type geometry;
		geometry.track_count = track_count;

		EXPECT_EQ(
			generic_disk_image(create_stream(buffer_stream_, geometry), geometry).track_count(),
			geometry.track_count);
	}
}

// Validates the sector count returned is the same as the one specified in the geometry
TEST_F(test_generic_disk_image, sector_count_property)
{
	for (const auto head_count : test_value_sequence_)
	{
		geometry_type geometry;
		geometry.head_count = head_count;

		EXPECT_EQ(
			generic_disk_image(create_stream(buffer_stream_, geometry), geometry).head_count(),
			geometry.head_count);
	}
}

// Validates the first valid sector id returned is the same as the one specified passed
// to the constructor
TEST_F(test_generic_disk_image, first_valid_sector_id_property)
{
	EXPECT_EQ(
		generic_disk_image(create_stream(buffer_stream_, test_geometry_), test_geometry_, 0, 0).first_valid_sector_id(),
		0);

	EXPECT_EQ(
		generic_disk_image(create_stream(buffer_stream_, test_geometry_), test_geometry_, 0, 1).first_valid_sector_id(),
		1);

	EXPECT_EQ(
		generic_disk_image(create_stream(buffer_stream_, test_geometry_), test_geometry_, 0, 5).first_valid_sector_id(),
		5);
}

// Validates the size of each sector on the disk can be retrieved.
TEST_F(test_generic_disk_image, get_sector_size)
{
	iterate_geometry(
		generic_disk_image{ create_stream(buffer_stream_, test_geometry_), test_geometry_},
		buffer_stream_,
		test_geometry_,
		[](generic_disk_image& image,
		   [[maybe_unused]] memory_stream_buffer& stream_buffer,
		   const geometry_type& geometry,
		   size_type head,
		   size_type track,
		   size_type sector)
		{
			ASSERT_EQ(
				image.get_sector_size(
					head,
					track,
					head,
					track,
					sector),
				geometry.sector_size);
		});
}

// Validates get sector size throws on invalid arguments
TEST_F(test_generic_disk_image, get_sector_size_throws_on_invalid_arguments)
{
	generic_disk_image image(create_stream(buffer_stream_, test_geometry_), test_geometry_);

	// Pass invalid disk head
	EXPECT_THROW(
		(void)image.get_sector_size(
			test_geometry_.head_count,
			0, // drive track
			0, // head id
			0, // track id
			image.first_valid_sector_id()),
		std::invalid_argument);

	// Pass invalid disk track
	EXPECT_THROW(
		(void)image.get_sector_size(
			0, // drive head
			test_geometry_.track_count,
			0, // head id
			0, // track id
			image.first_valid_sector_id()),
		std::invalid_argument);

	// Pass invalid head id
	EXPECT_THROW(
		(void)image.get_sector_size(
			0, // drive head
			0, // drive track
			test_geometry_.head_count,
			0, // track id
			image.first_valid_sector_id()),
		std::invalid_argument);

	// Pass invalid track id
	EXPECT_THROW(
		(void)image.get_sector_size(
			0, // drive head
			0, // drive track
			0, // head id
			test_geometry_.track_count,
			image.first_valid_sector_id()),
		std::invalid_argument);

	// Pass invalid sector id
	EXPECT_THROW(
		(void)image.get_sector_size(
			0, // drive head
			0, // drive track
			0, // head id
			0, // track id
			test_geometry_.sector_count + image.first_valid_sector_id()),
		std::invalid_argument);
}

// Validates sector record headers can be read for all valid sectors on the disk.
TEST_F(test_generic_disk_image, query_sector_header_by_index_returns_all_known_headers)
{
	generic_disk_image image(create_stream(buffer_stream_, test_geometry_), test_geometry_);

	for (auto head(0u); head < test_geometry_.head_count; ++head)
	{
		for (auto track(0u); track < test_geometry_.track_count; ++track)
		{
			for (auto sector(image.first_valid_sector_id());
				 sector < test_geometry_.sector_count + image.first_valid_sector_id();
				 ++sector)
			{
				const auto record(image.query_sector_header_by_index(head, track, sector));

				EXPECT_TRUE(record.has_value());
				EXPECT_EQ(record->head_id, head);
				EXPECT_EQ(record->track_id, track);
				EXPECT_EQ(record->sector_id, sector);
			}
		}
	}
}

// Validates the query throws an exception on invalid arguments..
TEST_F(test_generic_disk_image, query_sector_header_by_index_throws_on_invalid_arguments)
{
	generic_disk_image image(create_stream(buffer_stream_, test_geometry_), test_geometry_);

	EXPECT_THROW(
		((void)image.query_sector_header_by_index(test_geometry_.head_count, 0, image.first_valid_sector_id())),
		std::invalid_argument);

	EXPECT_THROW(
		((void)image.query_sector_header_by_index(0, test_geometry_.track_count, image.first_valid_sector_id())),
		std::invalid_argument);

	EXPECT_GT(image.first_valid_sector_id(), 0u);
	EXPECT_THROW(
		((void)image.query_sector_header_by_index(0, test_geometry_.track_count, 0)),
		std::invalid_argument);
	EXPECT_THROW(
		((void)image.query_sector_header_by_index(
			0,
			0,
			test_geometry_.sector_count + image.first_valid_sector_id() + 1)),
		std::invalid_argument);
}

// Validates all sector can be properly written to.
TEST_F(test_generic_disk_image, read_sector)
{
	const geometry_type geometry{ 2, 8, 16, 16 };
	const auto first_sector_id = 1;

	generic_disk_image image(
		create_stream(buffer_stream_, geometry),
		geometry,
		0,
		first_sector_id,
		false);

	iterate_geometry(image, buffer_stream_, geometry, fill_sector);
	iterate_geometry(image, buffer_stream_, geometry, validate_sector_read);
}

TEST_F(test_generic_disk_image, read_sector_throws_on_invalid_arguments)
{
	generic_disk_image image(create_stream(buffer_stream_, test_geometry_), test_geometry_);
	buffer_type sector_buffer;

	// Pass invalid disk head
	EXPECT_THROW(
		image.read_sector(
			test_geometry_.head_count,
			0, // drive track
			0, // head id
			0, // track id
			image.first_valid_sector_id(),
			sector_buffer),
		std::invalid_argument);

	// Pass invalid disk track
	EXPECT_THROW(
		image.read_sector(
			0, // drive head
			test_geometry_.track_count,
			0, // head id
			0, // track id
			image.first_valid_sector_id(),
			sector_buffer),
		std::invalid_argument);

	// Pass invalid head id
	EXPECT_THROW(
		image.read_sector(
			0, // drive head
			0, // drive track
			test_geometry_.head_count,
			0, // track id
			image.first_valid_sector_id(),
			sector_buffer),
		std::invalid_argument);

	// Pass invalid track id
	EXPECT_THROW(
		image.read_sector(
			0, // drive head
			0, // drive track
			0, // head id
			test_geometry_.track_count,
			image.first_valid_sector_id(),
			sector_buffer),
		std::invalid_argument);

	// Pass invalid sector id
	EXPECT_THROW(
		image.read_sector(
			0, // drive head
			0, // drive track
			0, // head id
			0, // track id
			test_geometry_.sector_count + image.first_valid_sector_id(),
			sector_buffer),
		std::invalid_argument);
}

// Validates all sector can be properly read.
TEST_F(test_generic_disk_image, write_sector)
{
	const geometry_type geometry{ 2, 8, 16, 16 };
	const auto first_sector_id = 1;

	generic_disk_image image(
		create_stream(buffer_stream_, geometry),
		geometry,
		0,
		first_sector_id,
		false);

	std::fill(buffer_stream_.begin(), buffer_stream_.end(), 0xff);
	iterate_geometry(image, buffer_stream_, geometry, validate_sector_write);
	iterate_geometry(image, buffer_stream_, geometry, validate_sector_data);
}

TEST_F(test_generic_disk_image, read_track)
{
	// TODO: implementation of read_track is pending. Tests deferred.
}

TEST_F(test_generic_disk_image, write_track)
{
	// TODO: completion of write_track is pending. Tests deferred.
}

TEST_F(test_generic_disk_image, write_track_throws_on_invalid_arguments)
{
	generic_disk_image image(create_stream(buffer_stream_, test_geometry_), test_geometry_);
	buffer_type sector_buffer;

	// Pass invalid disk head
	EXPECT_THROW(
		image.write_sector(
			test_geometry_.head_count,
			0, // drive track
			0, // head id
			0, // track id
			image.first_valid_sector_id(),
			sector_buffer),
		std::invalid_argument);

	// Pass invalid disk track
	EXPECT_THROW(
		image.write_sector(
			0, // drive head
			test_geometry_.track_count,
			0, // head id
			0, // track id
			image.first_valid_sector_id(),
			sector_buffer),
		std::invalid_argument);

	// Pass invalid head id
	EXPECT_THROW(
		image.write_sector(
			0, // drive head
			0, // drive track
			test_geometry_.head_count,
			0, // track id
			image.first_valid_sector_id(),
			sector_buffer),
		std::invalid_argument);

	// Pass invalid track id
	EXPECT_THROW(
		image.write_sector(
			0, // drive head
			0, // drive track
			0, // head id
			test_geometry_.track_count,
			image.first_valid_sector_id(),
			sector_buffer),
		std::invalid_argument);

	// Pass invalid sector id
	EXPECT_THROW(
		image.write_sector(
			0, // drive head
			0, // drive track
			0, // head id
			0, // track id
			test_geometry_.sector_count + image.first_valid_sector_id(),
			sector_buffer),
		std::invalid_argument);

	// Pass invalid sector id
	EXPECT_THROW(
		image.write_sector(
			0, // drive head
			0, // drive track
			0, // head id
			0, // track id
			image.first_valid_sector_id(),
			sector_buffer),
		vcc::media::buffer_size_error);
}

TEST_F(test_generic_disk_image, write_track_throws_on_write_protected_disks)
{
	generic_disk_image image(create_stream(buffer_stream_, test_geometry_), test_geometry_, 0, 1, true);
	buffer_type sector_buffer(test_geometry_.sector_size);

	// Pass invalid disk head
	EXPECT_THROW(
		image.write_sector(
			0, // drive head
			0, // drive track
			0, // head id
			0, // track id
			image.first_valid_sector_id(),
			sector_buffer),
		vcc::media::write_protect_error);
}
