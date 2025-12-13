#pragma once
#include <vcc/peripherals/disk_drives/generic_disk_drive.h>
#include <stdexcept>


namespace vcc::peripherals::disk_drives
{

	generic_disk_drive::generic_disk_drive(disk_image_ptr_type placeholder_disk)
		:
		placeholder_disk_(move(placeholder_disk)),
		disk_image_(placeholder_disk_)
	{
		if (placeholder_disk_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Basic Disk Drive. The placeholder disk image is null.");
		}
	}

	bool generic_disk_drive::has_disk() const noexcept
	{
		return !file_path_.empty();
	}

	generic_disk_drive::path_type generic_disk_drive::file_path() const
	{
		return file_path_;
	}

	bool generic_disk_drive::is_write_protected() const noexcept
	{
		return disk_image_->is_write_protected();
	}

	generic_disk_drive::size_type generic_disk_drive::head_count() const noexcept
	{
		return disk_image_->head_count();

	}

	generic_disk_drive::size_type generic_disk_drive::track_count() const noexcept
	{
		return disk_image_->track_count();
	}

	generic_disk_drive::size_type generic_disk_drive::get_sector_count(size_type image_head, size_type image_track) const
	{
		return disk_image_->get_sector_count(image_head, image_track);
	}


	bool generic_disk_drive::is_valid_disk_head(size_type drive_head) const noexcept
	{
		return disk_image_->is_valid_disk_head(drive_head);
	}

	bool generic_disk_drive::is_valid_disk_track(size_type drive_track) const noexcept
	{
		return disk_image_->is_valid_disk_track(drive_track);
	}

	bool generic_disk_drive::is_valid_track_sector(
		size_type drive_head,
		size_type drive_track,
		size_type drive_sector) const noexcept
	{
		return disk_image_->is_valid_track_sector(drive_head, drive_track, drive_sector);
	}

	bool generic_disk_drive::is_valid_sector_record(
		size_type drive_head,
		size_type drive_track,
		size_type head_id,
		size_type track_id,
		size_type sector_id) const noexcept
	{
		return disk_image_->is_valid_sector_record(drive_head, drive_track, head_id, track_id, sector_id);
	}



	generic_disk_drive::size_type generic_disk_drive::get_sector_size(
		size_type drive_head,
		size_type head_id,
		size_type track_id,
		size_type sector_id) const
	{
		return disk_image_->get_sector_size(
			drive_head,
			head_position(),
			head_id,
			track_id,
			sector_id);
	}


	void generic_disk_drive::insert(disk_image_ptr_type disk_image, path_type file_path)
	{
		if (!disk_image)
		{
			throw std::invalid_argument("Cannot insert disk. Disk image is null.");
		}

		if (file_path.empty())
		{
			throw std::invalid_argument("Cannot insert disk. File path is empty.");
		}

		eject();

		disk_image_ = move(disk_image);
		file_path_ = std::move(file_path);
	}

	void generic_disk_drive::eject()
	{
		disk_image_ = placeholder_disk_;
		file_path_.clear();
	}


	generic_disk_drive::error_id_type generic_disk_drive::read_sector(
		size_type drive_head,
		size_type head_id,
		size_type track_id,
		size_type sector_id,
		buffer_type& data_buffer) const
	{
		return disk_image_->read_sector(
			drive_head,
			head_position(),
			head_id,
			track_id,
			sector_id,
			data_buffer);
	}

	generic_disk_drive::error_id_type generic_disk_drive::write_sector(
		size_type drive_head,
		size_type head_id,
		size_type track_id,
		size_type sector_id,
		const buffer_type& data_buffer) const
	{
		return disk_image_->write_sector(
			drive_head,
			head_position(),
			head_id,
			track_id,
			sector_id,
			data_buffer);
	}

	generic_disk_drive::sector_record_vector generic_disk_drive::read_track(
		size_type drive_head) const
	{
		return disk_image_->read_track(drive_head, head_position());
	}

	void generic_disk_drive::write_track(
		size_type drive_head,
		const std::vector<sector_record_type>& sectors) const
	{
		disk_image_->write_track(drive_head, head_position(), sectors);
	}


	generic_disk_drive::size_type generic_disk_drive::head_position() const
	{
		return head_position_;
	}

	std::optional<generic_disk_drive::sector_record_header_type> generic_disk_drive::query_sector_header_by_index(
		size_type drive_head,
		size_type drive_sector) const
	{
		return disk_image_->query_sector_header_by_index(drive_head, head_position(), drive_sector);
	}

	void generic_disk_drive::step(step_direction direction)
	{
		switch (direction)
		{
		case step_direction::in:
			step_in();
			break;

		case step_direction::out:
			step_out();
			break;
		}
	}

	void generic_disk_drive::step_in()
	{
		if (disk_image_->is_valid_disk_track(head_position_ + 1))
		{
			++head_position_;
		}
	}

	void generic_disk_drive::step_out()
	{
		if (head_position() > 0)
		{
			--head_position_;
		}
	}

	void generic_disk_drive::seek_to_track(size_type drive_track)
	{
		//	FIXME-CHET: Check for exceeding max track.
		head_position_ = drive_track;
	}

}
