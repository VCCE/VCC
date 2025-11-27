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
#include "vcc/media/disk_geometry.h"
#include "vcc/media/sector_record.h"
#include "vcc/detail/exports.h"
#include <optional>
#include <vector>


namespace vcc::media
{

	/// @brief Defines the shape of the Disk Image interface for accessing virtual disk
	/// images.
	/// 
	/// This abstract class defines the fundamental operations for reading and writing
	/// track and sector data stored in a file containing a virtual disk image.
	class LIBCOMMON_EXPORT disk_image
	{
	public:

		/// @brief Type alias to lengths, 1 dimension sizes, and indexes.
		using size_type = ::std::size_t;
		/// @brief The type used to hold geometry describing the disk capacity.
		using geometry_type = ::vcc::media::disk_geometry;
		/// @brief Specifies the type used to represent a sector record.
		using sector_record_type = ::vcc::media::sector_record;
		/// @brief Specifies the type used to represent a sector record header.
		using sector_record_header_type = sector_record_header;
		/// @brief The type of buffer used to store sector data.
		using buffer_type = std::vector<unsigned char>;
		/// @brief The type of a vector of sector records.
		using sector_record_vector = std::vector<sector_record_type>;


	public:

		/// @brief Constructs a Disk Image.
		/// 
		/// @todo this should thrown if either the track or head is 0.
		/// 
		/// @param geometry The geometry of the disk.
		/// @param first_valid_sector_id The first identifier that can be used for a sector.
		/// @param write_protected Specifies if the disk is write protected or not.
		/// 
		/// @throws std::invalid_argument The number of heads specified in the geometry is 0.
		/// @throws std::invalid_argument The number of tracks specified in the geometry is 0.
		explicit disk_image(
			const geometry_type& geometry = {},
			size_type first_valid_sector_id = 1,
			bool write_protected = false);

		virtual ~disk_image() = default;

		/// @brief Indicates the total number of heads in supported by the disk.
		/// 
		/// @return A value greater than 0 (zero) specifying the number of heads.
		[[nodiscard]] virtual size_type head_count() const noexcept;

		/// @brief Indicates the total number of tracks in supported by the disk.
		/// 
		/// @return A value greater than 0 (zero) specifying the number of heads.
		[[nodiscard]] virtual size_type track_count() const noexcept;

		/// @brief Retrieves the first valid sector id supported by the disk.
		/// 
		/// @todo: Different file formats store the sector id differently and requires it
		/// be handled here, however this can probably go away if the image implementation
		/// handles it internally and/or the primary consumer up the call-chain always uses
		/// a base 0 sector id - but that may not always work. This however may not be possible.
		/// 
		/// @return The first valid sector identifier.
		[[nodiscard]] virtual size_type first_valid_sector_id() const noexcept;

		/// @brief Indicates if the disk is write protected.
		/// 
		/// Indicates if the disk is in write protect mode and cannot be written to.
		/// 
		/// @return `true` if the disk is write protected; otherwise `false`.
		[[nodiscard]] virtual bool is_write_protected() const noexcept;


		/// @brief Retrieve the number of sectors available on a specific head and track.
		/// 
		/// Retrieves the number of sectors available on a specific head and track of the disk
		/// image. If either the head or the track do not exist in the image, an exception is
		/// thrown.
		/// 
		/// @param disk_head The head the sector is stored on.
		/// @param disk_track The track the sector is stored in.
		/// 
		/// @return The total number of sectors or 0 (zero) if the track has no sectors.
		/// 
		/// @throws std::invalid_argument if the head specified in `disk_head` does not exist.
		/// @throws std::invalid_argument if the track specified in `disk_track` does not exist.
		[[nodiscard]] virtual size_type get_sector_count(
			size_type disk_head,
			size_type disk_track) const;


		/// @brief Check if the disk image contains a specific head.
		/// 
		/// @param disk_head The head to validate.
		/// 
		/// @return `true` if the head exists on the disk; `false` otherwise.
		[[nodiscard]] virtual bool is_valid_disk_head(size_type disk_head) const noexcept;

		/// @brief Check if the disk image contains a specific track on a specific head.
		/// 
		/// @param disk_track The track to validate.
		/// 
		/// @return `true` if the track exists on the disk; `false` otherwise.
		[[nodiscard]] virtual bool is_valid_disk_track(size_type disk_track) const noexcept;

		/// @brief Check if the disk image contains a sector on a specific head and track.
		/// 
		/// Check if a sector at a specific 0 (zero) based index of a head and track exists in
		/// the disk.
		/// 
		/// @todo maybe expand on the index nature of the sector - is_valid_sector_index or is_valid_track_sector_index.
		/// 
		/// @param disk_head The head the track is on.
		/// @param disk_track The track to validate.
		/// @param disk_sector The zero based index of the sector to validate.
		/// 
		/// @return `true` if the track exists on the disk; `false` otherwise.
		[[nodiscard]] virtual bool is_valid_track_sector(
			size_type disk_head,
			size_type disk_track,
			size_type disk_sector) const noexcept;

		/// @brief Check if the disk image contains a specific sector record.
		/// 
		/// Check if a sector matching specific head, track, and sector identifiers exists
		/// on the disk.
		/// 
		/// @todo declare this as `noexcept` once the unchecked version of functions it uses are available.
		/// @todo possibly make public for read/write validation. Add to base class if moved.
		/// 
		/// @param disk_head The head the sector is stored on.
		/// @param disk_track The track the sector is stored in.
		/// @param head_id The head identifier assigned to the sector to check for.
		/// @param track_id The track identifier assigned to the sector to check for.
		/// @param sector_id The sector identifier assigned to the sector to check for.
		/// 
		/// @return `true` if the sector exists; `false` otherwise.
		[[nodiscard]] virtual bool is_valid_sector_record(
			size_type disk_head,
			size_type disk_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id) const noexcept = 0;


		/// @brief Retrieve the number of size in bytes of a specific sector.
		/// 
		/// Retrieves the size in bytes of a specific sector on a specific head and track of
		/// the disk. Every sector stored on the disk is stored in a sector record that
		/// includes identifying properties such as head, track, and sector ids that are
		/// different than the physical head and track of the disk. These properties are used
		/// to identify the sector to retrieve the size of.
		/// 
		/// If either the head or the track do not exist in the image, or if the track does not
		/// contain a sector that matches the specified identifiers an exception is thrown.
		/// 
		/// @param disk_head The head the sector is stored on.
		/// @param disk_track The track the sector is stored in.
		/// @param head_id The head identifier assigned to the sector to retrieve the size for.
		/// @param track_id The track identifier assigned to the sector to retrieve the size for.
		/// @param sector_id The sector identifier assigned to the sector to retrieve the size for.
		/// 
		/// @return The size in bytes of the specified sector.
		/// 
		/// @throws std::invalid_argument if the head specified in `disk_head` does not exist.
		/// @throws std::invalid_argument if the track specified in `disk_track` does not exist.
		/// @throws std::invalid_argument if a sector record matching the head, track, and sector identifiers
		/// does not exist.
		[[nodiscard]] virtual size_type get_sector_size(
			size_type disk_head,
			size_type disk_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id) const = 0;

		/// @brief Tries to retrieve the set of properties stored in a sector header.
		/// 
		/// @todo Add a sibling `query_sector_header_by_id` to fetch the record as usually
		/// requested by the disk controller.
		/// 
		/// @param disk_head The head the sector is stored on.
		/// @param disk_track The track the sector is stored in.
		/// @param disk_sector The 0 (zero) based index of the sector to retrieve
		/// properties for.
		/// @return The properties of the sector header if the sector exists; en empty value
		/// otherwise.
		/// @throws std::invalid_argument if the head specified in `disk_head` does not exist.
		/// @throws std::invalid_argument if the track specified in `disk_track` does not exist.
		/// @throws std::invalid_argument if the sector at the index specified in `disk_sector` does not exist.
		[[nodiscard]] virtual std::optional<sector_record_header_type> query_sector_header_by_index(
			size_type disk_head,
			size_type disk_track,
			size_type disk_sector) const = 0;

		/// @brief Reads a sector from the disk.
		/// 
		/// Reads the first sector on a specified head and track of the disk that matches the
		/// specific set of head, track, and sector identifiers, resizing the data buffer to
		/// the exact size of the sector.
		/// 
		/// @param disk_head The head the sector is stored on.
		/// @param disk_track The track the sector is stored in.
		/// @param head_id The head identifier assigned to the sector to read.
		/// @param track_id The track identifier assigned to the sector to read.
		/// @param sector_id The sector identifier assigned to the sector to read.
		/// @param data_buffer The buffer to store the sector data in.
		/// 
		/// @throws std::invalid_argument if the head specified in `disk_head` does not exist.
		/// @throws std::invalid_argument if the track specified in `disk_track` does not exist.
		/// @throws std::invalid_argument if a sector record matching the head, track, and sector identifiers
		virtual void read_sector(
			size_type disk_head,
			size_type disk_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id,
			buffer_type& data_buffer) = 0;

		/// @brief Writes a sector to the disk.
		/// 
		/// Writes a block of data to the first sector on a specified head and track of the
		/// disk that matches the specific set of head, track, and sector identifiers.
		/// 
		/// @param disk_head The head the sector is stored on.
		/// @param disk_track The track the sector is stored in.
		/// @param head_id The head identifier assigned to the sector to write.
		/// @param track_id The track identifier assigned to the sector to write.
		/// @param sector_id The sector identifier assigned to the sector to write.
		/// @param data_buffer The buffer containing the data to write to the sector.
		/// 
		/// @throws std::invalid_argument if the head specified in `disk_head` does not exist.
		/// @throws std::invalid_argument if the track specified in `disk_track` does not
		/// exist.
		/// @throws std::invalid_argument if a sector record matching the head, track,
		/// and sector identifiers.
		/// @throws vcc::media::buffer_size_error if the sector buffer is smaller than the
		/// sector it is being written to.
		/// @throws vcc::media::fatal_io_error If the file position of the sector data cannot
		/// be access.
		/// @throws vcc::media::write_protect_error If the disk image has its write protect
		/// flag enabled.
		/// @throws vcc::media::fatal_io_error If an IO error occurs while reading the sector
		/// data.
		virtual void write_sector(
			size_type disk_head,
			size_type disk_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id,
			const buffer_type& data_buffer) = 0;

		/// @brief Reads an entire set of sector records from a track on the disk.
		/// 
		/// @todo This function is not implemented yet.
		/// 
		/// @param disk_head The head the sector is stored on.
		/// @param disk_track The track the sector is stored in.
		/// 
		/// @return A collection of sector records in the same order they are stored in on the
		/// disk.
		/// 
		/// @throws std::invalid_argument if the head specified in `disk_head` does not exist.
		/// @throws std::invalid_argument if the track specified in `disk_track` does not exist.
		[[nodiscard]] virtual sector_record_vector read_track(
			size_type disk_head,
			size_type disk_track) = 0;

		/// @brief Writes an entire set of sector records to a track on the disk.
		/// 
		/// Writes an entire set of sector records, in the order they are stored in a
		/// the collection passed here, to a track on the disk.
		/// 
		/// @param disk_head The head the sector is stored on.
		/// @param disk_track The track the sector is stored in.
		/// @param sectors The collection of sectors to write.
		/// 
		/// @throws std::invalid_argument if the head specified in `disk_head` does not exist.
		/// @throws std::invalid_argument if the track specified in `disk_track` does not exist.
		/// @throws vcc::media::write_protect_error If the disk image has its write protect
		/// flag enabled.
		/// @throws vcc::media::sector_record_error If a sector record being written is
		/// encountered with invalid parameters.
		virtual void write_track(
			size_type disk_head,
			size_type disk_track,
			const sector_record_vector& sectors) = 0;


	protected:

		/// @brief Retrieve the number of sectors available on a specific head and track.
		/// 
		/// Retrieves the number of sectors available on a specific head and track of the disk
		/// image without validating the head and track parameters are valid.
		/// 
		/// @param disk_head The head the sector is stored on. This value must be a valid
		/// head in the disk image. If this value is not valid the behavior of the function
		/// is undefined.
		/// @param disk_track The track the sector is stored in. This value must be a valid
		/// head in the disk image. If this value is not valid the behavior of the function
		/// is undefined.
		/// 
		/// @return The total number of sectors or 0 (zero) if the track has no sectors.
		[[nodiscard]] virtual size_type get_sector_count_unchecked(
			size_type disk_head,
			size_type disk_track) const noexcept = 0;

	private:

		/// @brief The number of heads on the disk.
		const size_type head_count_;
		/// @brief The number of heads on each track.
		const size_type track_count_;
		/// @brief The first sector id allowed by the media.
		const size_type first_valid_sector_id_;
		/// @brief Flag indicating if the disk image is write protected.
		const bool write_protected_;
	};

}
