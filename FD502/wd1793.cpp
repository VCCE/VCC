////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute it and/or
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
#include "wd1793.h"
#include "wd1793defs.h"
#include <vcc/media/disk_images/placeholder_disk_image.h>
#include <vcc/utils/disk_image_loader.h>


namespace vcc::cartridges::fd502
{

	wd1793_device::wd1793_device(std::shared_ptr<::vcc::bus::expansion_port_bus> bus)
		:
		bus_(move(bus)),
		placeholder_drive_(std::make_unique<::vcc::media::disk_images::placeholder_disk_image>()),
		disk_drives_{
			disk_drive_device_type(std::make_unique<::vcc::media::disk_images::placeholder_disk_image>()),
			disk_drive_device_type(std::make_unique<::vcc::media::disk_images::placeholder_disk_image>()),
			disk_drive_device_type(std::make_unique<::vcc::media::disk_images::placeholder_disk_image>()),
			disk_drive_device_type(std::make_unique<::vcc::media::disk_images::placeholder_disk_image>()) }
	{
	}


	bool wd1793_device::is_disk_write_protected(drive_id_type drive_id) const
	{
		return disk_drives_.at(drive_id).is_write_protected();
	}

	wd1793_device::path_type wd1793_device::get_mounted_disk_filename(drive_id_type drive_id) const
	{
		return disk_drives_.at(drive_id).file_path();
	}


	void wd1793_device::write_control_register(unsigned char value)
	{
		control_register_ = value;
	}

	void wd1793_device::write_command_register(unsigned char value)
	{
		registers_.command = value;
		execute_command(value);
	}

	void wd1793_device::write_track_register(unsigned char value)
	{
		registers_.track = value;
	}

	void wd1793_device::write_sector_register(unsigned char value)
	{
		registers_.sector = value;
	}

	void wd1793_device::write_data_register(unsigned char value)
	{
		(this->*write_data_byte_function_)(value);
	}

	unsigned char wd1793_device::read_status_register() const
	{
		// TODO-CHET: According to the notes reading any of these registers clears bit 7 (halt enable)
		// in the control flags register.
		return registers_.status;
	}

	unsigned char wd1793_device::read_track_register() const
	{
		// TODO-CHET: According to the notes reading any of these registers clears bit 7 (halt enable)
		// in the control flags register.
		return registers_.track;
	}

	unsigned char wd1793_device::read_sector_register() const
	{
		// TODO-CHET: According to the notes reading any of these registers clears bit 7 (halt enable)
		// in the control flags register.
		return registers_.sector;
	}

	unsigned char wd1793_device::read_data_register()
	{
		// TODO-CHET: According to the notes reading any of these registers clears bit 7 (halt enable)
		// in the control flags register.
		return (this->*fetch_data_byte_function_)();
	}


	void wd1793_device::insert_disk(
		drive_id_type drive_id,
		std::unique_ptr<disk_image_type> disk_image,
		path_type file_path)
	{
		if (drive_id >= disk_drives_.size())
		{
			throw std::invalid_argument("Cannot insert disk. Drive ID is invalid.");
		}

		if (disk_image == nullptr)
		{
			throw std::invalid_argument("Cannot insert disk. Disk image is null.");
		}

		if (file_path.empty())
		{
			throw std::invalid_argument("Cannot insert disk. File path is empty.");
		}

		disk_drives_[drive_id].insert(move(disk_image), std::move(file_path));
	}

	void wd1793_device::eject_disk(drive_id_type drive_id)
	{
		if (drive_id >= disk_drives_.size())
		{
			throw std::invalid_argument("Cannot eject disk. Drive ID is invalid.");
		}

		disk_drives_[drive_id].eject();
	}


	//This gets called at the end of every scan line so the controller has accurate timing.
	void wd1793_device::update([[maybe_unused]] float delta)
	{
		// FIXME-CHET: This looks wrong since there's no way for the command to timeout if
		// the motor is shut off prematurely.
		if (control_register_.motor_enabled() == false)
		{
			return;
		}

		// TODO-CHET: BEGIN
		// 
		// The following block of code until the matching END comment should be moved
		// somewhere else. An obvious spot would be in the disk drive implementation with
		// some additional support to run all the drives with randomized index position
		// for each one with the index pulse, data, and other signals mixed.
		index_pulse_active_ = ++index_pulse_tick_counter_ > index_pulse_begin_tick_count_;
		if (index_pulse_tick_counter_ > index_pulse_end_tick_count_)
		{
			const auto wibble_ticks(rand() % max_index_pulse_wobble_tick_count_);

			index_pulse_active_ = false;
			index_pulse_tick_counter_ = 0;
			index_pulse_begin_tick_count_ = ticks_per_disk_rotation_ + wibble_ticks - index_pulse_duration_tick_count_;
			index_pulse_end_tick_count_ = ticks_per_disk_rotation_ + wibble_ticks;
		}

		auto& working_drive(get_selected_drive());
		// FIXME-CHET: This happens only for type I commands when a drive is connected. Find a better
		// way to express this (maybe add members is_type1, is_type2, etc.
		// FIXME-CHET: in this case even if the drive doesn't have a disk the drive can still spin
		// FIXME-CHET: If the drive doesn't have a disk wouldn't the index pulse always be on?
		if ((!current_command_.has_value() || current_command_->id() <= command_id_type::step_out_upd) && working_drive.has_disk())
		{
			if (index_pulse_active_)
			{
				registers_.status |= INDEXPULSE;
			}
			else
			{
				registers_.status &= ~INDEXPULSE;
			}
		}
		// TODO-CHET: END

		if (!current_command_.has_value())
		{
			return;
		}

		ExecTimeWaiter--;
		if (ExecTimeWaiter > 0)  //Click Click Click
		{
			return;
		}

		ExecTimeWaiter = 1;
		IOWaiter++;

		switch (current_command_->id())
		{
		case command_id_type::restore:
			if (working_drive.head_position() != 0)
			{
				working_drive.step_out();
				ExecTimeWaiter = (update_ticks_per_millisecond_ * current_command_->step_time());
			}
			else
			{
				registers_.track = 0;

				auto status_flags(READY | TRACK_ZERO);
				if (working_drive.is_write_protected())
				{
					status_flags |= WRITEPROTECT;
				}

				complete_command(status_flags);
			}
			break;

		case command_id_type::seek:
			// TODO: Make it clear that the data register contains the track to seek to
			if (working_drive.head_position() != registers_.data)
			{
				// FIXME-CHET: Check if we should determine the direction to step each time here
				// instead of using step_direction_.
				working_drive.step(step_direction_);

				ExecTimeWaiter = (update_ticks_per_millisecond_ * current_command_->step_time());
			}
			else
			{
				// FIXME-CHET: This seems wrong. During the seek operation the track register is
				// updated with the current head position. It may be updated on each step.
				working_drive.seek_to_track(registers_.track);

				auto status_flags(READY);
				if (working_drive.head_position() == 0)
				{
					status_flags |= TRACK_ZERO;
				}

				if (working_drive.is_write_protected())
				{
					status_flags |= WRITEPROTECT;
				}

				complete_command(status_flags);
			}
			break;

		case command_id_type::step:
		case command_id_type::step_upd:
			working_drive.step(step_direction_);

			if (current_command_->id() == command_id_type::step_upd)
			{
				if (step_direction_ == step_direction_type::in)
				{
					++registers_.track;
				}
				else
				{
					--registers_.track;
				}
			}

			{
				auto status_flags(READY);
				// FIXME-CHET: Prior to the refactor these two blocks came BEFORE
				// registers_.status was set to READY. Determine if they are
				// valid and need to be set
				if (working_drive.head_position() == 0)
				{
					//status_flags |= TRACK_ZERO;
				}

				if (working_drive.is_write_protected())
				{
					//status_flags |= WRITEPROTECT;
				}

				complete_command(status_flags);
			}
			break;

		case command_id_type::step_in:
		case command_id_type::step_in_upd:
			// We need to save the direction we are stepping in to support `step` commands that
			// may follow, which relies on knowing the direction of the last step command.
			step_direction_ = step_direction_type::in;

			working_drive.step_in();
			if (current_command_->id() == command_id_type::step_in_upd)
			{
				registers_.track += 1;
			}

			{
				auto status_flags(READY);
				// FIXME-CHET: Prior to the refactor the next block came BEFORE registers_.status was set
				// to READY. Determine if they are valid and need to be set
				if (working_drive.is_write_protected())
				{
					//status_flags |= WRITEPROTECT;
				}
				complete_command(status_flags);
			}
			break;

		case command_id_type::step_out:
		case command_id_type::step_out_upd:
			// We need to save the direction we are stepping in to support `step` commands that
			// may follow, which relies on knowing the direction of the last step command.
			step_direction_ = step_direction_type::out;

			working_drive.step_out();
			if (current_command_->id() == command_id_type::step_out_upd)
			{
				registers_.track -= 1;
			}

			{
				auto status_flags(READY);
				// FIXME-CHET: Prior to the refactor the next block came BEFORE registers_.status was set
				// to READY. Determine if they are valid and need to be set
				if (working_drive.head_position() == 0)
				{
					//status_flags |= TRACK_ZERO;
				}
				if (working_drive.is_write_protected())
				{
					//status_flags |= WRITEPROTECT;
				}

				complete_command(status_flags);
			}
			break;

		case command_id_type::read_sector:
		case command_id_type::read_sector_m:
			if (IOWaiter > default_io_wait_tick_count_)
			{
				data_loss_detected_ = true;
				read_byte_from_sector();
			}
			break;

		case command_id_type::write_sector:
		case command_id_type::write_sector_m:
			registers_.status |= DRQ;	//IRON
			if (IOWaiter > default_io_wait_tick_count_)
			{
				data_loss_detected_ = true;
				write_byte_to_sector(0);
			}
			break;

		case command_id_type::read_address:
			if (IOWaiter > default_io_wait_tick_count_)
			{
				data_loss_detected_ = true;
				read_byte_from_address();
			}
			break;

		case command_id_type::force_interupt:
			break;

		case command_id_type::read_track:
			if (IOWaiter > default_io_wait_tick_count_)
			{
				data_loss_detected_ = true;
				read_byte_from_track();
			}
			break;

		case command_id_type::write_track:
			registers_.status |= DRQ;	//IRON
			if (IOWaiter > default_io_wait_tick_count_)
			{
				data_loss_detected_ = true;
				write_byte_to_track(0);
			}
			break;
		}
	}

	void wd1793_device::execute_command(unsigned char command_packet)
	{
		const auto command(static_cast<command_id_type>(command_packet >> 4));
		if (current_command_.has_value() && command != command_id_type::force_interupt)
		{
			return;
		}

		const auto& working_drive(get_selected_drive());
		current_command_ = fdc_command_type(command, command_packet & 0x0f);

		switch (current_command_->id())
		{
		case command_id_type::restore:
			// FIXME-CHET: Shouldn't this be multiplied by the number of tracks needed to step to 0?
			ExecTimeWaiter = update_ticks_for_command_settle_ + (update_ticks_per_millisecond_ * current_command_->step_time());
			registers_.status = BUSY;
			break;

		case command_id_type::seek:
			// FIXME-CHET: This does not seem to account for a drive with the head position already
			// on the track the drive is seeking to. Do we need to?
			ExecTimeWaiter = update_ticks_for_command_settle_ + (update_ticks_per_millisecond_ * current_command_->step_time());
			registers_.track = registers_.data;
			registers_.status = BUSY;
			if (working_drive.head_position() > registers_.data)
			{
				step_direction_ = step_direction_type::out;
			}
			else
			{
				step_direction_ = step_direction_type::in;
			}
			break;

		case command_id_type::step:
		case command_id_type::step_upd:
			ExecTimeWaiter = update_ticks_for_command_settle_ + (update_ticks_per_millisecond_ * current_command_->step_time());
			registers_.status = BUSY;
			break;

		case command_id_type::step_in:
		case command_id_type::step_in_upd:
			ExecTimeWaiter = update_ticks_for_command_settle_ + (update_ticks_per_millisecond_ * current_command_->step_time());
			registers_.status = BUSY;
			break;

		case command_id_type::step_out:
		case command_id_type::step_out_upd:
			ExecTimeWaiter = update_ticks_for_command_settle_ + (update_ticks_per_millisecond_ * current_command_->step_time());
			registers_.status = BUSY;
			break;

		case command_id_type::read_sector:
		case command_id_type::read_sector_m:
			registers_.status = BUSY | DRQ;
			fetch_data_byte_function_ = &wd1793_device::read_byte_from_sector;
			ExecTimeWaiter = 1;
			IOWaiter = 0;
			break;

		case command_id_type::write_sector:
		case command_id_type::write_sector_m:
			registers_.status = BUSY;
			write_data_byte_function_ = &wd1793_device::write_byte_to_sector;
			ExecTimeWaiter = 5;
			IOWaiter = 0;
			// FIXME-CHET: This is a temporary fix until all write protect states are
			// handled properly. This should be handled in the command executor (update
			// function).
			if (working_drive.is_write_protected())
			{
				complete_command(WRITEPROTECT);
			}
			break;

		case command_id_type::read_address:
			registers_.status = BUSY | DRQ;
			fetch_data_byte_function_ = &wd1793_device::read_byte_from_address;
			ExecTimeWaiter = 1;
			IOWaiter = 0;
			break;

		case command_id_type::force_interupt:
			current_command_.reset();
			registers_.status = READY;
			ExecTimeWaiter = 1;
			// FIXME-CHET: This is incorrect. There are 4 different interrupt flags for
			// various conditions but only one is immediate and the new state of the
			// status register is determined by the flags and whether a command is
			// currently executing and some conditions are satisfied asynchronously
			// like generating an interrupt for index holes passing by.
			if ((command_packet & 15) != 0)
			{
				bus_->assert_nmi_interrupt_line();
			}
			break;

		case command_id_type::read_track:
			registers_.status = BUSY | DRQ;
			fetch_data_byte_function_ = &wd1793_device::read_byte_from_track;
			ExecTimeWaiter = 1;
			IOWaiter = 0;
			break;

		case command_id_type::write_track:
			registers_.status = BUSY;
			write_data_byte_function_ = &wd1793_device::write_byte_to_track;
			ExecTimeWaiter = 1;
			ExecTimeWaiter = 5;
			IOWaiter = 0;
			// FIXME-CHET: This is a temporary fix until all write protect states are
			// handled properly. This should be handled in the command executor (update
			// function).
			if (working_drive.is_write_protected())
			{
				complete_command(WRITEPROTECT);
			}
			break;
		}
	}


	unsigned char wd1793_device::read_byte_from_data_register()
	{
		return registers_.data;
	}

	unsigned char wd1793_device::read_byte_from_sector()
	{
		const auto& working_drive(get_selected_drive());

		if (!working_drive.has_disk())
		{
			complete_command(RECNOTFOUND);
			return 0;
		}

		// TODO-CHET: This is from the original implementation but is not correct. Once
		// the drive has stepped the head to the desired track, the track register can be
		// changed before the read command is initiated and is used to match the sector
		// record on the track. While checking the head position against the track
		// register here is OK for raw disk images like DSK (the vast majority of images)
		// it could break for DMK or similar formats where copy protection is a factor.
		// This would be better handled in the disk drive or image or a collaboration
		// among them.
		if (read_transfer_buffer_.empty() && registers_.track == working_drive.head_position())
		{
			// TODO-CHET: Right now it reads immediately but really shouldn't be available
			// until the sector is actually spinning by the head. 
			auto result(working_drive.read_sector(
				control_register_.head(),
				control_register_.head(),
				registers_.track,
				registers_.sector,
				read_transfer_buffer_));

			if (result != disk_error_id_type::success)
			{
				auto status_flags(0u);
				switch (result)
				{
				case disk_error_id_type::success:
					throw std::runtime_error("Cannot process success status in error handler.");

				case disk_error_id_type::empty:
					status_flags |= LOSTDATA;
					break;

				case disk_error_id_type::invalid_head:
				case disk_error_id_type::invalid_track:
				case disk_error_id_type::invalid_sector:
					status_flags |= RECNOTFOUND;
					break;

				case disk_error_id_type::write_protected:
					// TODO-CHET: Not sure how to handle this since there should never
					// be an instance where this occurs. Maybe throw.
					status_flags |= WRITEPROTECT;
					break;
				}

				complete_command(status_flags);
				return 0;
			}

			read_transfer_position_ = 0;
		}

		if (read_transfer_buffer_.empty())
		{
			complete_command(RECNOTFOUND);
			return 0;
		}

		unsigned char RetVal = 0;
		if (read_transfer_position_ < read_transfer_buffer_.size())
		{
			RetVal = read_transfer_buffer_[read_transfer_position_++];
			registers_.status = BUSY | DRQ;
			IOWaiter = 0;
		}
		else
		{
			complete_command(READY);
			if (data_loss_detected_)
			{
				registers_.status = LOSTDATA;
				data_loss_detected_ = false;
			}
			registers_.sector++;
		}

		return RetVal;
	}

	unsigned char wd1793_device::read_byte_from_address()
	{
		unsigned char RetVal = 0;

		const auto& working_drive(get_selected_drive());

		if (read_transfer_buffer_.empty())
		{
			// FIXME-CHET: This isn't quite correct. If accurate timing is expected this
			// needs to take into account the interleave and other factors to get an
			// accurate sector to select from. For drive image formats like DMK which can
			// have sectors in any order some type of mechanism to determine the next
			// sector will be need.
			const auto drive_sector(index_pulse_tick_counter_ / 176);
			const auto sector_record(working_drive.query_sector_header_by_index(control_register_.head(), drive_sector));
			if (!sector_record.has_value())
			{
				complete_command(RECNOTFOUND);
				return 0;
			}

			read_transfer_buffer_.resize(6);
			read_transfer_buffer_[0] = static_cast<unsigned char>(sector_record->head_id);
			read_transfer_buffer_[1] = static_cast<unsigned char>(sector_record->track_id);
			read_transfer_buffer_[2] = static_cast<unsigned char>(sector_record->sector_id);
			read_transfer_buffer_[3] = get_id_from_sector_size(sector_record->sector_length);
			read_transfer_buffer_[4] = 0;	//	TODO-CHET: Need to calculate the CRC for
			read_transfer_buffer_[5] = 0;	//	bytes 4 and 5

			read_transfer_position_ = 0;
		}

		if (!working_drive.has_disk())
		{
			complete_command(RECNOTFOUND);
			return 0;
		}

		if (read_transfer_position_ < read_transfer_buffer_.size())
		{
			RetVal = read_transfer_buffer_[read_transfer_position_++];
			registers_.status = BUSY | DRQ;
			IOWaiter = 0;
		}
		else
		{
			complete_command(READY);
			if (data_loss_detected_)
			{
				registers_.status = LOSTDATA;
				data_loss_detected_ = false;
			}
		}

		return RetVal;
	}

	unsigned char wd1793_device::read_byte_from_track()
	{
#if defined(INCLUDE_BROKEN_SHIT)
		unsigned char RetVal = 0;

		if (TransferBufferSize == 0)
		{
			//const auto& working_drive(disk_drives_[current_drive_]);

			//TransferBufferSize = working_drive.read_track(current_head_, working_drive.head_position(), TransferBuffer.data());
			//		WriteLog("BEGIN read_track",0);
		}

		if (TransferBufferSize == 0)//iron | (registers_.track != disk_drives_[CurrentDrive].head_position()) ) //| (registers_.sector > disk_drives_[CurrentDrive].sector_count_)
		{
			complete_command(RECNOTFOUND);
			return 0;
		}

		if (TransferBufferIndex < TransferBufferSize)
		{
			RetVal = TransferBuffer[TransferBufferIndex++];
			registers_.status = BUSY | DRQ;
			IOWaiter = 0;
		}
		else
		{
			//		WriteLog("read_track DONE",0);
			complete_command(READY);
			if (data_loss_detected_)
			{
				registers_.status = LOSTDATA;
				data_loss_detected_ = false;
			}

			// TODO-CHET: Does the sector register actually change here or
			// is it the track register that needs to be incremented?
			++registers_.sector;
		}

		return RetVal;
#else
		complete_command(RECNOTFOUND);
		return 0;
#endif
	}


	void wd1793_device::write_byte_to_data_register(unsigned char value)
	{
		registers_.data = value;
	}

	void wd1793_device::write_byte_to_sector(unsigned char value)
	{
		if (write_transfer_buffer_.empty())
		{
			// TODO-CHET: The disk image will resize the buffer to fit a full sector in
			// regardless of its size. We're setting it to a default of 256 here and will
			// rely on the result of calling the write_sector function to report record
			// not found. We may want to just query this but probably not.
			write_transfer_buffer_.resize(256);
			write_transfer_position_ = 0;
		}

		if (write_transfer_position_ == write_transfer_buffer_.size())
		{
			const auto& working_drive(get_selected_drive());

			const auto result(working_drive.write_sector(
				control_register_.head(),
				control_register_.head(),
				registers_.track,
				registers_.sector,
				write_transfer_buffer_));

			auto status_flags(READY);

			switch (result)
			{
			case disk_error_id_type::success:
				break;

			case disk_error_id_type::empty:
				status_flags |= LOSTDATA;
				break;

			case disk_error_id_type::invalid_head:
			case disk_error_id_type::invalid_track:
			case disk_error_id_type::invalid_sector:
				status_flags |= RECNOTFOUND;
				break;

			case disk_error_id_type::write_protected:
				status_flags |= WRITEPROTECT;
			}

			// TODO-CHET: Can data loss really be reported here?
			if (data_loss_detected_)
			{
				status_flags |= LOSTDATA;
			}

			// TODO-CHET: The status returned by the disk drive may be sufficient. If not
			// we should check the write-protect state before going the write.
			if (working_drive.is_write_protected())
			{
				// TODO-CHET: I didn't see anything in the documentation about setting
				// record not found. Validate this is the right behavior.
				status_flags |= WRITEPROTECT | RECNOTFOUND;
			}

			complete_command(status_flags);
			data_loss_detected_ = false;
			++registers_.sector;
		}
		else
		{
			write_transfer_buffer_[write_transfer_position_++] = value;
			registers_.status = BUSY | DRQ;
			IOWaiter = 0;
		}
	}

	void wd1793_device::write_byte_to_track(unsigned char value)
	{
		if (write_transfer_buffer_.empty())
		{
			write_transfer_buffer_.resize(6272);
			write_transfer_position_ = 0;
		}

		// FIXME-CHET: should this wait for an extra byte to be written to the port before
		// the data is written and the command completed? 
		if (write_transfer_position_ == write_transfer_buffer_.size())
		{
			const auto& working_drive(get_selected_drive());

			const auto result = write_raw_track(
				working_drive,
				control_register_.head(),
				write_transfer_buffer_.data(),
				write_transfer_buffer_.size());

			auto status_flags(READY);
			if (result == false || data_loss_detected_ == true)
			{
				status_flags |= LOSTDATA;
				data_loss_detected_ = false;
			}

			if (working_drive.is_write_protected())
			{
				status_flags |= WRITEPROTECT | RECNOTFOUND;
			}

			complete_command(status_flags);
		}
		else
		{
			write_transfer_buffer_[write_transfer_position_++] = value;
			registers_.status = BUSY | DRQ;
			IOWaiter = 0;
		}
	}

	bool wd1793_device::write_raw_track(
		const disk_drive_device_type& disk_drive,
		head_id_type drive_head,
		const unsigned char* data_buffer,
		size_type data_buffer_size) const
	{
		// FIXME-CHET: This should do more validation to determine if what is being written
		// is valid and will fit on the disk.

		std::vector<disk_image_type::sector_record_type> sectors;
		size_type writing_track = 0;
		size_type writing_head = 0;
		size_type writing_sector = 0;
		size_type writing_sector_size = 0;

		for (auto data_offset(0u); data_offset < data_buffer_size; data_offset++)
		{
			if (data_buffer[data_offset] == 0xFE) //	Look for ID.A.M.
			{
				writing_track = data_buffer[data_offset + 1];
				writing_head = data_buffer[data_offset + 2];
				writing_sector = data_buffer[data_offset + 3];
				writing_sector_size = get_sector_size_from_id(data_buffer[data_offset + 4]);
				data_offset += 6;
			}

			if (data_buffer[data_offset] == 0xFB)	//	Look for D.A.M.
			{
				// FIXME-CHET: This causes the data offset to go 1 byte past the sector data for the
				// next iteration of the loop. Validate and document why we're incrementing it.
				++data_offset;

				sectors.push_back({
					writing_head,
					writing_track,
					writing_sector,
					writing_sector_size,
					disk_image_type::buffer_type(&data_buffer[data_offset], &data_buffer[data_offset + writing_sector_size]) });

				data_offset += writing_sector_size;
			}
		}

		// FIXME-CHET: Should this take the track from the current head position of the
		// drive? or should that have been handled before getting here.
		disk_drive.write_track(drive_head, sectors);

		return true;
	}

	wd1793_device::size_type wd1793_device::drive_count() const
	{
		return disk_drives_.size();
	}


	bool wd1793_device::is_motor_running() const noexcept
	{
		return control_register_.motor_enabled();
	}

	std::optional<wd1793_device::drive_id_type> wd1793_device::get_selected_drive_id() const noexcept
	{
		if (!control_register_.has_drive_selection())
		{
			return {};
		}

		return control_register_.selected_drive();
	}

	wd1793_device::head_id_type wd1793_device::get_selected_head() const noexcept
	{
		return control_register_.head();
	}


	wd1793_device::size_type wd1793_device::get_head_position() const noexcept
	{
		return get_selected_drive().head_position();
	}

	wd1793_device::size_type wd1793_device::get_current_sector() const noexcept
	{
		return registers_.sector;
	}


	void wd1793_device::set_turbo_mode(bool enable_turbo)
	{
		turbo_enabled_ = enable_turbo;
		if (!turbo_enabled_)
		{
			update_ticks_for_command_settle_ = default_update_ticks_for_command_settle_;
			update_ticks_per_millisecond_ = default_update_ticks_per_millisecond_;
		}
		else
		{
			update_ticks_for_command_settle_ = 1;
			update_ticks_per_millisecond_ = 0;
		}
	}

	void wd1793_device::complete_command(std::optional<unsigned char> status_flags)
	{
		// FIXME-CHET: Validate there is a command executing and throw if not.
		if (status_flags.has_value())
		{
			registers_.status = status_flags.value();
		}

		if (control_register_.interupt_enabled())
		{
			bus_->assert_nmi_interrupt_line();
		}

		fetch_data_byte_function_ = &wd1793_device::read_byte_from_data_register;
		write_data_byte_function_ = &wd1793_device::write_byte_to_data_register;
		current_command_.reset();
		read_transfer_buffer_.resize(0);
		write_transfer_buffer_.resize(0);
	}


	wd1793_device::size_type wd1793_device::get_sector_size_from_id(sector_size_id_type id) const
	{
		return 0x80 << id;
	}

	wd1793_device::sector_size_id_type wd1793_device::get_id_from_sector_size(size_type size) const
	{
		return static_cast<wd1793_device::sector_size_id_type>(size >> 7);
	}


	wd1793_device::disk_drive_device_type& wd1793_device::get_selected_drive()
	{
		if (control_register_.has_drive_selection())
		{
			return disk_drives_[control_register_.selected_drive()];
		}

		return placeholder_drive_;
	}

	const wd1793_device::disk_drive_device_type& wd1793_device::get_selected_drive() const
	{
		if (control_register_.has_drive_selection())
		{
			return disk_drives_[control_register_.selected_drive()];
		}

		return placeholder_drive_;
	}


	void wd1793_device::start()
	{
	}


	void wd1793_device::stop()
	{
		for (auto& drive : disk_drives_)
		{
			drive.eject();
		}

		// TODO_CHET: Finish resetting to a default state.
	}

}
