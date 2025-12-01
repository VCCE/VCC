#include <vcc/utils/disk_image_loader.h>
#include <vcc/media/disk_images/generic_disk_image.h>
#include <vcc/media/geometry/generic_disk_geometry.h>
#include <vcc/utils/streams.h>
#include <array>
#include <fstream>


namespace vcc::utils
{

	namespace
	{

		std::size_t get_sector_size_from_id(unsigned int id)
		{
			return 0x80 << id;
		}

	}


	std::unique_ptr<::vcc::media::disk_image> load_disk_image(const std::filesystem::path& file_path)
	{
		std::array<unsigned char, 256> header_buffer;
		long TotalSectors = 0;
		unsigned char TmpSides = 0;
		unsigned char TmpSectors = 0;
		unsigned char TmpMod = 0;


		auto write_protected = false;
		auto image_stream(std::make_unique<std::fstream>());

		// Read the header
		image_stream->open(file_path, std::ios::binary | std::ios::in | std::ios::out);
		if (!image_stream->is_open())
		{	//Can't open read/write might be read only
			image_stream->open(file_path, std::ios::binary | std::ios::in);
			write_protected = true;
		}

		if (!image_stream->is_open())
		{
			return {};
		}

		const auto file_size(::vcc::utils::get_stream_size(*image_stream.get()));
		if (file_size < header_buffer.size())
		{
			return {};
		}

		image_stream->read(reinterpret_cast<char*>(header_buffer.data()), header_buffer.size());
		if (image_stream->fail())
		{
			return {};
		}

		auto header_size = file_size % 256;
		auto first_valid_sector_id = 1u;

		::vcc::media::geometry::generic_disk_geometry geometry;
		// TODO-CHET: This needs to calculate the number of tracks available (35, 40, or 80. More for hard disks)
		switch (header_size)
		{
		case 4:
			first_valid_sector_id = header_buffer[3];

		case 3:
			geometry.sector_size = get_sector_size_from_id(header_buffer[2]);

		case 2:
			geometry.sector_count = header_buffer[0];
			geometry.head_count = header_buffer[1];

		case 0:
			//OS9 Image checks
			TotalSectors = (header_buffer[0] << 16) + (header_buffer[1] << 8) + (header_buffer[2]);
			TmpSides = (header_buffer[16] & 1) + 1;
			TmpSectors = header_buffer[3];
			TmpMod = 1;
			if ((TmpSides * TmpSectors) != 0)
			{
				TmpMod = TotalSectors % (TmpSides * TmpSectors);
			}

			if (TmpSectors == 18 && TmpMod == 0 && header_size == 0)	//Sanity Check 
			{
				geometry.head_count = TmpSides;
				geometry.sector_count = TmpSectors;
			}
			break;

		case 12:
			//working_drive.ImageType = floppy_disk_geometry::VDK;	//VDK
			geometry.head_count = header_buffer[9];
			break;

		case 0xFF:
			break;

		default:
			return {};
		}

		return std::make_unique<::vcc::media::disk_images::generic_disk_image>(
			move(image_stream),
			geometry,
			header_size,
			first_valid_sector_id,
			write_protected);
	}

}
