#include "test_disk_image.h"
#include "vcc/media/exceptions.h"


// Validates the constructor throws an exception on invalid arguments.
TEST_F(test_disk_image, invalid_ctor_arguments)
{
	// Must throw on 0 heads
	EXPECT_THROW(disk_image({ 0, 1 }), std::invalid_argument);

	// Must throw on 0 tracks
	EXPECT_THROW(disk_image({ 1, 0 }), std::invalid_argument);
}

// Validates the head count returned is the same as the one specified in the geometry
TEST_F(test_disk_image, head_count_property)
{
	for (const auto head_count : test_value_sequence_)
	{
		geometry_type geometry;
		geometry.head_count = head_count;

		EXPECT_EQ(disk_image(geometry).head_count(), geometry.head_count);
	}
}

// Validates the sector count returned is the same as the one specified in the geometry
TEST_F(test_disk_image, sector_count_property)
{
	for (const auto track_count : test_value_sequence_)
	{
		geometry_type geometry;
		geometry.track_count = track_count;

		EXPECT_EQ(disk_image(geometry).track_count(), geometry.track_count);
	}
}

// Validates the first valid sector id returned is the same as the one specified passed
// to the constructor
TEST_F(test_disk_image, first_valid_sector_id_property)
{
	EXPECT_EQ(disk_image(test_geometry_, 0).first_valid_sector_id(), 0);
	EXPECT_EQ(disk_image(test_geometry_, 1).first_valid_sector_id(), 1);
	EXPECT_EQ(disk_image(test_geometry_, 5).first_valid_sector_id(), 5);
}

// Validates the head count returned is the same value passed to the constructor.
TEST_F(test_disk_image, is_write_protected)
{
	EXPECT_TRUE(disk_image(test_geometry_, 0, true).is_write_protected());
	EXPECT_FALSE(disk_image(test_geometry_, 0, false).is_write_protected());
}

// Validates the head validation
TEST_F(test_disk_image, is_valid_disk_head)
{
	geometry_type geometry;
	geometry.head_count = *std::max_element(test_value_sequence_.begin(), test_value_sequence_.end()) + 1;

	for (const auto head : test_value_sequence_)
	{
		EXPECT_TRUE(disk_image(geometry).is_valid_disk_head(head));
	}
}

// Validates the head validation return false on invalid head
TEST_F(test_disk_image, is_valid_disk_head_fails_on_invalid_head)
{
	geometry_type geometry;
	geometry.head_count = 1;

	EXPECT_FALSE(disk_image(geometry).is_valid_disk_head(geometry.head_count + 1));
	EXPECT_FALSE(disk_image(geometry).is_valid_disk_head(geometry.head_count + 2));
	EXPECT_FALSE(disk_image(geometry).is_valid_disk_head(geometry.head_count + 10));
}

// Validates the track validation
TEST_F(test_disk_image, is_valid_disk_track)
{
	geometry_type geometry;
	geometry.track_count = *std::max_element(test_value_sequence_.begin(), test_value_sequence_.end()) + 1;

	for (const auto track : test_value_sequence_)
	{
		EXPECT_TRUE(disk_image(geometry).is_valid_disk_track(track));
	}
}

// Validates the track validation failed on invalid track
TEST_F(test_disk_image, is_valid_disk_track_fails_on_invalid_track)
{
	geometry_type geometry;
	geometry.track_count = 1;

	EXPECT_FALSE(disk_image(geometry).is_valid_disk_head(geometry.track_count + 1));
	EXPECT_FALSE(disk_image(geometry).is_valid_disk_head(geometry.track_count + 2));
	EXPECT_FALSE(disk_image(geometry).is_valid_disk_head(geometry.track_count + 10));
}
