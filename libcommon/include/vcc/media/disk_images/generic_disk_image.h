#pragma once
#include <vcc/media/disk_image.h>
#include <vcc/media/geometry/generic_disk_geometry.h>
#include <Windows.h>
#include <iostream>
#include <memory>


namespace vcc::media::disk_images
{

	/// @brief Basic implementation of the Disk Image interface.
	/// 
	/// This implementation of the Disk Image interface provides support for basic disk
	/// image files that use a fixed number of tracks and sectors each with a fixed size.
	/// It supports disk image files with track and sector data starting anywhere in the
	/// file but requires that it be stored contiguously.
	class generic_disk_image : public disk_image
	{
	public:

		/// @brief The type used to hold geometry describing the disk capacity.
		using geometry_type = ::vcc::media::geometry::generic_disk_geometry;
		/// @brief The type of stream used to access the disk image file.
		using stream_type = ::std::iostream;
		/// @brief The type used to hold a pointer to the stream used to access the disk image file.
		using stream_ptr_type = ::std::unique_ptr<stream_type>;
		/// @brief The type used to store a position in the disk image file.
		using position_type = stream_type::pos_type;


	public:

		/// @brief Constructs a Basic Disk Image.
		/// 
		/// @param stream The IO stream used to access disk image file.
		/// @param geometry The geometry of the disk.
		/// @param track_data_offset AN offset from the beginning of the file that points
		/// to the location of the track and sector data.
		/// @param first_valid_sector_id The first identifier that can be used for a sector.
		/// @param write_protected Specifies if the disk is write protected or not.
		/// 
		/// @throws std::invalid_argument if the stream is null.
		/// @throws std::invalid_argument if the stream is a file stream and is not open.
		LIBCOMMON_EXPORT generic_disk_image(
			stream_ptr_type stream,
			const geometry_type& geometry,
			position_type track_data_offset = 0,
			size_type first_valid_sector_id = 1,
			bool write_protected = false);

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT bool is_valid_sector_record(
			size_type disk_head,
			size_type disk_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id) const noexcept override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT size_type get_sector_size(
			size_type disk_head,
			size_type disk_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id) const override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT std::optional<sector_record_header_type> query_sector_header_by_index(
			size_type disk_head,
			size_type disk_track,
			size_type disk_sector) const override;

		/// @inheritdoc
		LIBCOMMON_EXPORT void read_sector(
			size_type disk_head,
			size_type disk_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id,
			buffer_type& data_buffer) override;

		/// @inheritdoc
		LIBCOMMON_EXPORT void write_sector(
			size_type disk_head,
			size_type disk_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id,
			const buffer_type& data_buffer) override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT sector_record_vector read_track(
			size_type disk_head,
			size_type disk_track) override;

		/// @inheritdoc
		LIBCOMMON_EXPORT void write_track(
			size_type disk_head,
			size_type disk_track,
			const sector_record_vector& sectors) override;


	protected:

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT size_type get_sector_count_unchecked(
			size_type disk_head,
			size_type disk_track) const noexcept override;

		/// @brief Move the file pointer to a specific location in the image file.
		/// 
		/// @param position The position to move to.
		/// 
		/// @return `true` if the file pointer was successfully changed; `false` otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT bool seek(const position_type& position);

		/// @brief Calculates the file position of a specific sector on the disk.
		/// 
		/// Calculates the file position of a specific sector on the disk. This
		/// function does not validate arguments specifying the sector to locate and
		/// assumes they are correct.
		/// 
		/// @param disk_head The head the sector is stored on.
		/// @param disk_track The track the sector is stored in.
		/// @param head_id The head identifier assigned to the sector to read.
		/// @param track_id The track identifier assigned to the sector to read.
		/// @param sector_id The sector identifier assigned to the sector to read.
		/// 
		/// @return The file position of the specified sector.
		[[nodiscard]] LIBCOMMON_EXPORT position_type calculate_sector_offset_unchecked(
			size_type disk_head,
			size_type disk_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id) const noexcept;


	private:

		const stream_ptr_type stream_ptr_;
		stream_type& stream_;
		const position_type file_size_;
		const position_type track_data_offset_;
		const size_type sector_count_;
		const size_type track_size_;
		const size_type sector_size_;
	};

}
