#include "vcc/utils/cartridge_path.h"



namespace vcc::utils
{

	cartridge_path::cartridge_path(path_type path)
		: path_or_id_(std::move(path))
	{
		if (get<cartridge_class_id::rom>(path_or_id_).empty())
		{
			throw std::invalid_argument("Cannot construct Cartridge Path. File path is empty.");
		}
	}

	cartridge_path::cartridge_path(guid_type id)
		: path_or_id_(id)
	{ }

	bool cartridge_path::is_rom_cartridge() const noexcept
	{
		return path_or_id_.index() == cartridge_class_id::rom;
	}

	bool cartridge_path::is_device_cartridge() const noexcept
	{
		return path_or_id_.index() == cartridge_class_id::device;
	}

	cartridge_path::path_type cartridge_path::rom_path() const
	{
		if (!is_rom_cartridge())
		{
			throw std::runtime_error("Cannot retrieve ROM path. Cartridge is not a ROM.");
		}

		return get<cartridge_class_id::rom>(path_or_id_);
	}

	cartridge_path::guid_type cartridge_path::device_id() const
	{
		if (!is_device_cartridge())
		{
			throw std::runtime_error("Cannot retrieve ROM path. Cartridge is not a device.");
		}

		return get<cartridge_class_id::device>(path_or_id_);
	}

	std::optional<cartridge_path> cartridge_path::from_string(const string_type& text)
	{
		if (text.starts_with("rom:"))
		{
			return cartridge_path(text.substr(4));
		}

		if (text.starts_with("id:"))
		{
			const auto id(guid_type::from_string(text.substr(3)));

			if (!id.has_value())
			{
				return {};
			}

			return cartridge_path(id.value());
		}

		return {};
	}

	cartridge_path::string_type cartridge_path::to_string() const
	{
		switch (path_or_id_.index())
		{
		case cartridge_class_id::empty:
			return {};

		case cartridge_class_id::rom:
			return "rom:" + get<cartridge_class_id::rom>(path_or_id_).string();

		case cartridge_class_id::device:
			return "id:" + get<cartridge_class_id::device>(path_or_id_).to_string();
		}

		return {};
	}


}
