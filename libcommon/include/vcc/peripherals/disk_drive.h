#pragma once
#include <vcc/detail/exports.h>
#include <vcc/peripherals/step_direction.h>
#include <vcc/media/disk_geometry.h>
#include <vcc/media/disk_error_id.h>
#include <vcc/media/sector_record.h>
#include <filesystem>
#include <vector>
#include <optional>
#include <string>


// TODO-CHET: Maybe move to vcc::devices or vcc::devices::peripherals or somewhere
namespace vcc::peripherals
{

	/// @brief Disk Drive interface
	///
	/// The basic Disk Drive interface defines the properties and operations of a basic disk
	/// drive.
	/// 
	/// todo rename track to cylinder
	class LIBCOMMON_EXPORT disk_drive
	{
	public:

		/// @brief Type alias to lengths, 1 dimension sizes, and indexes.
		using size_type = std::size_t;
		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;
		/// @brief TYpe alias for a sector record
		using sector_record_type = ::vcc::media::sector_record;
		/// @brief TYpe alias for a sector record header
		using sector_record_header_type = ::vcc::media::sector_record_header;
		/// @brief TYpe alias for a vector sector records
		using sector_record_vector = std::vector<sector_record_type>;
		/// @brief TYpe alias for the buffer used to store and transfer sector data.
		using buffer_type = std::vector<unsigned char>;
		/// @brief Type alias for error codes returned by various functions.
		using error_id_type = ::vcc::media::disk_error_id;


	public:

		virtual ~disk_drive() = default;

		/// @brief Checks if the disk has a disk image inserted.
		/// 
		/// @return `true` if a disk image is inserted into the disk; `false` otherwise.
		[[nodiscard]] virtual bool has_disk() const noexcept = 0;

		/// @brief Indicates the total number of heads in supported by the disk drive.
		/// 
		/// @return A value greater than 0 (zero) specifying the number of heads.
		[[nodiscard]] virtual size_type head_count() const noexcept = 0;

		/// @brief Indicates the total number of tracks in supported by the disk drive.
		/// 
		/// @return A value greater than 0 (zero) specifying the number of heads.
		[[nodiscard]] virtual size_type track_count() const noexcept = 0;

		/// @brief Indicates if the disk currently inserted in the drive is write protected.
		/// 
		/// Indicates if the disk is write protected and whether it can be written to.
		/// 
		/// @return `true` if the disk is write protected; otherwise `false`.
		[[nodiscard]] virtual bool is_write_protected() const noexcept = 0;

		/// @brief Retrieve the number of sectors available on a specific head and track.
		/// 
		/// Retrieves the number of sectors available on a specific head and track of the disk
		/// image currently inserted in the drive.
		/// 
		/// @param drive_head The head the sector is stored on.
		/// @param drive_track The track the sector is stored in.
		/// 
		/// @return The total number of sectors or 0 (zero) if the track has no sectors.
		[[nodiscard]] virtual size_type get_sector_count(size_type drive_head, size_type drive_track) const = 0;

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
		/// @todo this may need some throws added.
		/// 
		/// @param drive_head The head the sector is stored on.
		/// @param head_id The head identifier assigned to the sector to retrieve the size for.
		/// @param track_id The track identifier assigned to the sector to retrieve the size for.
		/// @param sector_id The sector identifier assigned to the sector to retrieve the size for.
		/// 
		/// @return The size in bytes of the specified sector.
		[[nodiscard]] virtual size_type get_sector_size(
			size_type drive_head,
			size_type head_id,
			size_type track_id,
			size_type sector_id) const = 0;

		/// @brief Retrieves the current position of the disk head.
		/// 
		/// @return The current position of the disk head.
		[[nodiscard]] virtual size_type head_position() const = 0;

		/// @brief Check if the disk image contains a specific head.
		/// 
		/// @param drive_head The head to validate.
		/// 
		/// @return `true` if the head exists on the disk; `false` otherwise.
		[[nodiscard]] virtual bool is_valid_disk_head(size_type drive_head) const noexcept = 0;

		/// @brief Check if the disk image contains a specific track on a specific head.
		/// 
		/// @param drive_track The track to validate.
		/// 
		/// @return `true` if the track exists on the disk; `false` otherwise.
		[[nodiscard]] virtual bool is_valid_disk_track(size_type drive_track) const noexcept = 0;

		/// @brief Check if the disk image contains a sector on a specific head and track.
		/// 
		/// Check if a sector at a specific 0 (zero) based index of a head and track exists in
		/// the disk.
		/// 
		/// @todo maybe expand on the index nature of the sector - is_valid_sector_index or is_valid_track_sector_index.
		/// 
		/// @param drive_head The head the track is on.
		/// @param drive_track The track to validate.
		/// @param drive_sector The zero based index of the sector to validate.
		/// 
		/// @return `true` if the track exists on the disk; `false` otherwise.
		[[nodiscard]] virtual bool is_valid_track_sector(
			size_type drive_head,
			size_type drive_track,
			size_type drive_sector) const noexcept = 0;

		/// @brief Check if the disk image contains a specific sector record.
		/// 
		/// Check if a sector matching specific head, track, and sector identifiers exists
		/// on the disk.
		/// 
		/// @todo declare this as `noexcept` once the unchecked version of functions it uses are available.
		/// @todo possibly make public for read/write validation. Add to base class if moved.
		/// 
		/// @param drive_head The head the sector is stored on.
		/// @param drive_track The track the sector is stored in.
		/// @param head_id The head identifier assigned to the sector to check for.
		/// @param track_id The track identifier assigned to the sector to check for.
		/// @param sector_id The sector identifier assigned to the sector to check for.
		/// 
		/// @return `true` if the sector exists; `false` otherwise.
		[[nodiscard]] virtual bool is_valid_sector_record(
			size_type drive_head,
			size_type drive_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id) const noexcept = 0;

		/// @brief Move the head to a specific track.
		/// 
		/// Moves the read/write head to a specific track.
		/// 
		/// @param drive_track The track to move the head to.
		/// 
		/// @throws std::invalid_argument if the track specified in `drive_track` does not exist.
		virtual void seek_to_track(size_type drive_track) = 0;

		/// @brief Move the head to one track either in or out.
		/// 
		/// This moves the head by one in a specific direction.
		/// 
		/// @todo this should throw an exception if the step can't be performed
		/// @todo this should probably send events on the move.
		/// 
		/// @param direction The direction to move the head.
		virtual void step(step_direction direction) = 0;

		/// @brief Moves the head towards the tracks at the inside of the disk.
		virtual void step_in() = 0;

		/// @brief Moves the head towards the tracks at the outside of the disk.
		virtual void step_out() = 0;


		/// @brief Reads a sector from the disk.
		/// 
		/// Reads the first sector on a specified head and track of the disk that matches the
		/// specific set of head, track, and sector identifiers, resizing the data buffer to
		/// the exact size of the sector.
		/// 
		/// @todo this may need some throws added.
		/// 
		/// @param drive_head The head the sector is stored on.
		/// @param head_id The head identifier assigned to the sector to read.
		/// @param track_id The track identifier assigned to the sector to read.
		/// @param sector_id The sector identifier assigned to the sector to read.
		/// @param data_buffer The buffer to store the sector data in.
		virtual error_id_type read_sector(
			size_type drive_head,
			size_type head_id,
			size_type track_id,
			size_type sector_id,
			buffer_type& data_buffer) const = 0;

		/// @brief Writes a sector to the disk.
		/// 
		/// Writes a block of data to the first sector on a specified head and track of the
		/// disk that matches the specific set of head, track, and sector identifiers.
		/// 
		/// @todo this may need some throws added.
		/// 
		/// @param drive_head The head the sector is stored on.
		/// @param head_id The head identifier assigned to the sector to write.
		/// @param track_id The track identifier assigned to the sector to write.
		/// @param sector_id The sector identifier assigned to the sector to write.
		/// @param data_buffer The buffer containing the data to write to the sector.
		virtual error_id_type write_sector(
			size_type drive_head,
			size_type head_id,
			size_type track_id,
			size_type sector_id,
			const buffer_type& data_buffer) const = 0;

		/// @brief Reads an entire set of sector records from a track on the disk.
		/// 
		/// @todo This function is not implemented yet.
		/// @todo this may need some throws added.
		/// 
		/// @param drive_head The head the sector is stored on.
		/// 
		/// @return A collection of sector records in the same order they are stored in on the
		/// disk.
		[[nodiscard]] virtual sector_record_vector read_track(size_type drive_head) const = 0;

		/// @brief Writes an entire set of sector records to a track on the disk.
		/// 
		/// Writes an entire set of sector records, in the order they are stored in a
		/// the collection passed here, to a track on the disk.
		/// 
		/// @todo this may need some throws added.
		/// 
		/// @param drive_head The head the sector is stored on.
		/// @param sectors The collection of sectors to write.
		virtual void write_track(
			size_type drive_head,
			const sector_record_vector& sectors) const = 0;

		/// @brief Tries to retrieve the set of properties stored in a sector header.
		/// 
		/// @todo Add a sibling `query_sector_header_by_id` to fetch the record as usually
		/// requested by the disk controller.
		/// @todo: This is used to fetch the next sector from the rotation offset for floppy
		/// drives but doesn't seem to meet expectations in its current corm. Need to make this
		/// more cohesive between the client, the disk drive, and the disk image.
		/// @todo this may need some throws added.
		/// 
		/// 
		/// @param drive_head The head the sector is stored on.
		/// @param drive_sector The 0 (zero) based index of the sector to retrieve
		/// properties for.
		/// 
		/// @return The properties of the sector header if the sector exists; en empty value
		/// otherwise.
		[[nodiscard]] virtual std::optional<sector_record_header_type> query_sector_header_by_index(
			size_type drive_head,
			size_type drive_sector) const = 0;
	};

}
