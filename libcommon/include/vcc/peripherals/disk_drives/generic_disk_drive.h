#pragma once
#include <vcc/peripherals/disk_drive.h>
#include <vcc/media/disk_image.h>
#include <vcc/media/disk_geometry.h>
#include <queue>
#include <string>
#include <array>
#include <memory>


namespace vcc::peripherals::disk_drives
{

	/// @brief Generic Disk Drive
	///
	/// The Generic Disk Drive implementation provides a basic implementation of the Disk
	/// Drive interface. The Generic Drive manages drive relative properties such as the
	/// current head and operations such as seeking between tracks but relies on the disk
	/// image to define the geometry and overall capabilities of I/O.
	class generic_disk_drive : public disk_drive
	{
	public:

		/// @brief Type alias for the disk images this disk drive operates on.
		using disk_image_type = ::vcc::media::disk_image;
		/// @brief Type alias for a pointer to a disk image with single ownership.
		using disk_image_ptr_type = std::unique_ptr<::vcc::media::disk_image>;
		/// @brief Type alias for a shared pointer to the disk images this disk drive operates on.
		using shared_disk_image_ptr_type = std::shared_ptr<::vcc::media::disk_image>;
		/// @brief Type alias for the geometry that defines the storage parameters of the drive.
		using geometry_type = ::vcc::media::disk_geometry;


	public:

		/// @brief Construct the Basic Disk Drive
		/// 
		/// @param placeholder_disk A pointer to the disk image to use as a placeholder when
		/// no disk is inserted into the drive.
		/// 
		/// @throws std::invalid_argument if `placeholder_disk` is null.
		explicit LIBCOMMON_EXPORT generic_disk_drive(disk_image_ptr_type placeholder_disk);

		/// @brief Insert a disk image into the disk.
		/// 
		/// @param disk_image A pointer to the disk image to insert.
		/// @param file_path The file path to the disk image.
		/// 
		/// @throws std::invalid_argument if `disk_image` is null.
		/// @throws std::invalid_argument if `file_path` is empty.
		LIBCOMMON_EXPORT virtual void insert(disk_image_ptr_type disk_image, path_type file_path);

		/// @brief Eject the disk.
		LIBCOMMON_EXPORT virtual void eject();

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT bool has_disk() const noexcept override;

		/// @brief Retrieve the file path to the disk image.
		/// 
		/// @return The file path of the disk image.
		[[nodiscard]] LIBCOMMON_EXPORT virtual path_type file_path() const;


		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT size_type head_count() const noexcept override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT size_type track_count() const noexcept override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT bool is_write_protected() const noexcept override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT size_type get_sector_count(size_type drive_head, size_type drive_track) const override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT bool is_valid_disk_head(size_type drive_head) const noexcept override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT bool is_valid_disk_track(size_type drive_track) const noexcept override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT bool is_valid_track_sector(
			size_type drive_head,
			size_type drive_track,
			size_type drive_sector) const noexcept override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT bool is_valid_sector_record(
			size_type drive_head,
			size_type drive_track,
			size_type head_id,
			size_type track_id,
			size_type sector_id) const noexcept override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT size_type get_sector_size(
			size_type drive_head,
			size_type head_id,
			size_type track_id,
			size_type sector_id) const override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT size_type head_position() const override;


		/// @inheritdoc
		LIBCOMMON_EXPORT void seek_to_track(size_type drive_track) override;

		/// @inheritdoc
		LIBCOMMON_EXPORT void step(step_direction direction) override;

		/// @inheritdoc
		LIBCOMMON_EXPORT void step_in() override;

		/// @inheritdoc
		LIBCOMMON_EXPORT void step_out() override;


		/// @inheritdoc
		LIBCOMMON_EXPORT void read_sector(
			size_type drive_head,
			size_type head_id,
			size_type track_id,
			size_type sector_id,
			buffer_type& data_buffer) const override;

		/// @inheritdoc
		LIBCOMMON_EXPORT void write_sector(
			size_type drive_head,
			size_type head_id,
			size_type track_id,
			size_type sector_id,
			const buffer_type& data_buffer) const override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT sector_record_vector read_track(size_type drive_head) const override;

		/// @inheritdoc
		LIBCOMMON_EXPORT void write_track(
			size_type drive_head,
			const sector_record_vector& sectors) const override;

		/// @inheritdoc
		[[nodiscard]] LIBCOMMON_EXPORT std::optional<sector_record_header_type> query_sector_header_by_index(
			size_type drive_head,
			size_type drive_sector) const override;


	private:

		/// @brief The placeholer disk image to use when there is no disk inserted.
		const shared_disk_image_ptr_type placeholder_disk_;
		/// @brief Pointer to the inserted disk image or the placeholder disk if no disk is inserted.
		shared_disk_image_ptr_type disk_image_;
		/// @brief The file path to the disk image if one is inserted.
		path_type file_path_;
		/// @brief The current position of the drive head.
		size_type head_position_ = 0;
	};

}
