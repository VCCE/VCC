#include <vcc/media/geometry_calculators/floppy_disk_geometry_calculator.h>
#include <vcc/media/disk_images/generic_disk_image.h>
#include <vcc/media/geometry/generic_disk_geometry.h>
#include <vcc/utils/disk_image_loader.h>
#include <vcc/utils/streams.h>
#include <array>
#include <fstream>


namespace vcc::utils
{


	std::unique_ptr<::vcc::media::disk_image> load_disk_image(const std::filesystem::path& file_path)
	{
		using geometry_calculator_type = ::vcc::media::geometry_calculators::floppy_disk_geometry_calculator;

		auto write_protected = false;
		auto image_stream(std::make_unique<std::fstream>());

		// Read the header
		image_stream->open(file_path, std::ios::binary | std::ios::in | std::ios::out);
		if (!image_stream->is_open())
		{	//Can't open read/write might be read only
			image_stream->open(file_path, std::ios::binary | std::ios::in);
			if (!image_stream->is_open())
			{
				return {};
			}

			write_protected = true;
		}

		geometry_calculator_type::header_buffer_type header_buffer;
		const auto file_size(::vcc::utils::get_stream_size(*image_stream.get()));
		if (file_size < header_buffer.size() || file_size > std::numeric_limits<std::size_t>::max())
		{
			return {};
		}

		image_stream->read(reinterpret_cast<char*>(header_buffer.data()), header_buffer.size());
		if (image_stream->fail())
		{
			return {};
		}

		const auto first_valid_sector_id = 1u;

		// TODO-CHET: This should be passed as am argument
		const geometry_calculator_type::geometry_type default_geometry;

		geometry_calculator_type calculator(default_geometry);
		auto geometry(calculator.calculate(header_buffer, static_cast<size_t>(file_size)));
		if (!geometry.has_value())
		{
			return {};
		}

		return std::make_unique<::vcc::media::disk_images::generic_disk_image>(
			move(image_stream),
			geometry->geometry,
			geometry->image_file_data_offset,
			first_valid_sector_id,
			write_protected);
	}

}
