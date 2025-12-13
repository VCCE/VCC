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
#include "vcc/media/disk_images/generic_disk_image.h"
#include "vcc/media/exceptions.h"
#include "vcc/utils/streams.h"
#include <fstream>


namespace vcc::media::disk_images
{

	namespace
	{
		generic_disk_image::stream_ptr_type validate_stream_argument(generic_disk_image::stream_ptr_type stream)
		{
			if (!stream)
			{
				throw std::invalid_argument("Cannot construct Basic Disk Image. Stream is null.");
			}

			return stream;
		}
	}


	generic_disk_image::generic_disk_image(
		stream_ptr_type stream,
		const geometry_type& geometry,
		position_type track_data_offset,
		size_type first_valid_sector_id,
		bool write_protected)
		:
		disk_image(geometry, first_valid_sector_id, write_protected),
		stream_ptr_(validate_stream_argument(move(stream))),
		stream_(*stream_ptr_.get()),
		file_size_(::vcc::utils::get_stream_size(stream_)),
		track_data_offset_(move(track_data_offset)),
		sector_count_(geometry.sector_count),
		track_size_(geometry.sector_count * geometry.sector_size),
		sector_size_(geometry.sector_size)
	{
		// TODO-CHET: find a way to do this without the cast. i.e. the file size is <0 if
		// the file is closed. Also check good/bad flags.
		if (const auto file_stream(dynamic_cast<std::fstream*>(&stream_));
			file_stream && !file_stream->is_open())
		{
			throw std::invalid_argument("Cannot construct Basic Disk Image. File stream is closed.");
		}

		if (file_size_ < 0)
		{
			throw std::runtime_error("Cannot construct Basic Disk Image. Stream size cannot be determined.");
		}

#ifdef INCLUDE_INCOMPLETE_AND_BROKEN_LIBCOMMON_CODE
		// FIXME: The solution for buffer streams is incomplete and causes tellg() and tellp()
		// to return incorrect values. Once buffer stream support is complete this code and its
		// associated should be compiled in.
		const auto minimum_image_data_size(head_count_ * track_count_ * sector_count_);
		const auto minimum_image_file_size(track_data_offset_ + position_type(minimum_image_data_size));
		if (file_size_ < minimum_image_file_size)
		{
			throw std::runtime_error("Cannot construct Basic Disk Image. Stream size is too small for specified geometry.");
		}
#endif
	}


	bool generic_disk_image::is_valid_sector_record(
		size_type disk_head,
		size_type disk_track,
		size_type head_id,
		size_type track_id,
		size_type sector_id) const noexcept
	{
		if (disk_head != head_id || disk_track != track_id)
		{
			return false;
		}

		if (sector_id < first_valid_sector_id())
		{
			return false;
		}

		return sector_id - first_valid_sector_id() < get_sector_count_unchecked(disk_head, disk_track);
	}


	generic_disk_image::size_type generic_disk_image::get_sector_size(
		size_type disk_head,
		size_type disk_track,
		size_type head_id,
		size_type track_id,
		size_type sector_id) const
	{
		if (!is_valid_disk_head(disk_head))
		{
			throw std::invalid_argument("Cannot retrieve the size of a sector. Specified drive head is invalid.");
		}

		if (!is_valid_disk_track(disk_track))
		{
			throw std::invalid_argument("Cannot retrieve the size of a sector. Specified drive track is invalid.");
		}

		if (!is_valid_sector_record(disk_head, disk_track, head_id, track_id, sector_id))
		{
			throw std::invalid_argument("Cannot retrieve the size of a sector. Specified sector id is invalid.");
		}

		return sector_size_;
	}

	std::optional<generic_disk_image::sector_record_header_type> generic_disk_image::query_sector_header_by_index(
		size_type disk_head,
		size_type disk_track,
		size_type disk_sector) const
	{
		if (!is_valid_disk_head(disk_head))
		{
			throw std::invalid_argument("Cannot retrieve the record header by index. Specified drive head is invalid.");
		}

		if (!is_valid_disk_track(disk_track))
		{
			throw std::invalid_argument("Cannot retrieve the record header by index. Specified drive track is invalid.");
		}

		if (disk_sector >= get_sector_count_unchecked(disk_head, disk_track) + first_valid_sector_id())
		{
			throw std::invalid_argument("Cannot retrieve the record header by index. Specified drive sector is invalid.");
		}

		return sector_record_header_type{
			disk_head,
			disk_track,
			disk_sector,
			sector_size_
		};
	}

	generic_disk_image::error_id_type generic_disk_image::read_sector(
		size_type disk_head,
		size_type disk_track,
		size_type head_id,
		size_type track_id,
		size_type sector_id,
		buffer_type& data_buffer)
	{
		if (!is_valid_disk_head(disk_head) || !is_valid_disk_head(head_id))
		{
			return error_id_type::invalid_head;
		}

		if (!is_valid_disk_track(disk_track) || !is_valid_disk_track(track_id))
		{
			return error_id_type::invalid_track;
		}

		if (!is_valid_sector_record(disk_head, disk_track, head_id, track_id, sector_id))
		{
			return error_id_type::invalid_sector;
		}

		if (!seek(calculate_sector_offset_unchecked(disk_head, disk_track, head_id, track_id, sector_id)))
		{
			throw fatal_io_error("Cannot read sector. Fatal IO error encountered while seeking to sector position.");
		}

		// TODO-CHET: There needs to be unchecked versions of sector_size() and other similar
		// functions since we're already checking the input parameters here.
		data_buffer.resize(get_sector_size(
			disk_head,
			disk_track,
			head_id,
			track_id,
			sector_id));

		stream_.clear();
		stream_.read(reinterpret_cast<char*>(data_buffer.data()), data_buffer.size());
		if (stream_.bad())
		{
			// FIXME-CHET: This should throw
			std::fill(data_buffer.begin(), data_buffer.end(), buffer_type::value_type(0xffu));
		}

		return error_id_type::success;
	}

	generic_disk_image::error_id_type generic_disk_image::write_sector(
		size_type disk_head,
		size_type disk_track,
		size_type head_id,
		size_type track_id,
		size_type sector_id,
		const buffer_type& data_buffer)
	{
		if (!is_valid_disk_head(disk_head) || !is_valid_disk_head(head_id))
		{
			return error_id_type::invalid_head;
		}

		if (!is_valid_disk_track(disk_track) || !is_valid_disk_track(track_id))
		{
			return error_id_type::invalid_track;
		}

		if (!is_valid_sector_record(disk_head, disk_track, head_id, track_id, sector_id))
		{
			return error_id_type::invalid_sector;
		}

		const auto sector_size(get_sector_size(disk_head, disk_track, head_id, track_id, sector_id));
		if (sector_size > data_buffer.size())
		{
			// TODO-CHET: Determine if we should write a partial sector here or if a write_partial_sector
			// should be added and used by the client.
			throw buffer_size_error("Cannot write sector. Sector buffer is smaller than the sector.");
		}

		if (!seek(calculate_sector_offset_unchecked(disk_head, disk_track, head_id, track_id, sector_id)))
		{
			throw fatal_io_error("Cannot write sector. Fatal IO error encountered while seeking to sector position.");
		}

		if (is_write_protected())
		{
			return error_id_type::write_protected;
		}

		stream_.write(reinterpret_cast<const char*>(data_buffer.data()), sector_size);
		stream_.flush();
		if (stream_.bad())
		{
			throw fatal_io_error("Cannot write sector. Fatal IO error encountered while writing sector.");
		}

		return error_id_type::success;
	}


	generic_disk_image::sector_record_vector generic_disk_image::read_track(
		size_type disk_head,
		size_type disk_track)
	{
		if (!is_valid_disk_head(disk_head))
		{
			throw std::invalid_argument("Cannot read track. Specified drive head is invalid.");
		}

		if (!is_valid_disk_track(disk_track))
		{
			throw std::invalid_argument("Cannot read track. Specified drive track is invalid.");
		}

		throw std::runtime_error("Cannot read track. Functionality is not implemented.");
	}

	void generic_disk_image::write_track(
		size_type disk_head,
		size_type disk_track,
		const sector_record_vector& sectors)
	{
		if (!is_valid_disk_head(disk_head))
		{
			throw std::invalid_argument("Cannot write track. Specified drive head is invalid.");
		}

		if (!is_valid_disk_track(disk_track))
		{
			throw std::invalid_argument("Cannot write track. Specified drive track is invalid.");
		}

		if (is_write_protected())
		{
			throw write_protect_error("Cannot write track. Disk is write protected.");
		}

		// TODO-CHET: This needs a more robust solution in order to support derived disk images
		// that manage sector record details in their format. We should
		//  - check for valid sector id's
		//  - check for sequential sector id's
		for (const auto& sector : sectors)
		{
			if (sector.header.sector_length != sector_size_)
			{
				throw sector_record_error("Cannot write track. One or more sector records have an unsupported or incorrect size.");
			}
		}

		auto last_result(error_id_type::success);
		for (const auto& sector : sectors)
		{
			// TODO-CHET: This is a placeholder. A more robust solution is needed to support other disk
			// formats
			const auto result(write_sector(
				disk_head,
				disk_track,
				sector.header.head_id,
				sector.header.track_id,
				sector.header.sector_id,
				sector.data));
			if (result != error_id_type::success)
			{
				last_result = result;
			}
		}

		//return last_result;
	}


	generic_disk_image::size_type generic_disk_image::get_sector_count_unchecked(
		[[maybe_unused]] size_type disk_head,
		[[maybe_unused]] size_type disk_track) const noexcept
	{
		return sector_count_;
	}


	bool generic_disk_image::seek(const position_type& position)
	{
		stream_.clear();
		return !stream_.seekg(position, std::ios::beg).bad()
			&& !stream_.seekp(position, std::ios::beg).bad();
	}


	generic_disk_image::position_type generic_disk_image::calculate_sector_offset_unchecked(
		size_type disk_head,
		size_type disk_track,
		[[maybe_unused]] size_type head_id,
		[[maybe_unused]] size_type track_id,
		size_type sector_id) const noexcept
	{
		// Calculate the base offset to the requested track data with tracks interleaved
		// with the heads (i.e. track 0 side A = image track 0, track 0 side B = image
		// track 1).
		const auto base_offset(head_count() * disk_track * track_size_);
		const auto track_offset(disk_head * track_size_);
		const auto sector_offset((sector_id - first_valid_sector_id()) * sector_size_);

		return track_data_offset_ + position_type(base_offset + track_offset + sector_offset);
	}

}
