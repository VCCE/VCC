#include "vcc/utils/resource_location.h"


namespace vcc::utils
{

	resource_location::resource_location(path_type path)
		: path_or_id_(std::move(path))
	{
		if (get<location_class_id_type::system_path>(path_or_id_).empty())
		{
			throw std::invalid_argument("Cannot construct Resource Location. System file path is empty.");
		}
	}

	resource_location::resource_location(guid_type id)
		: path_or_id_(id)
	{ }

	bool resource_location::is_path() const noexcept
	{
		return path_or_id_.index() == location_class_id_type::system_path;
	}

	bool resource_location::is_guid() const noexcept
	{
		return path_or_id_.index() == location_class_id_type::system_guid;
	}

	resource_location::path_type resource_location::path() const
	{
		if (!is_path())
		{
			throw std::runtime_error("Cannot retrieve path. Resource Location is not a path.");
		}

		return get<location_class_id_type::system_path>(path_or_id_);
	}

	resource_location::guid_type resource_location::guid() const
	{
		if (!is_guid())
		{
			throw std::runtime_error("Cannot retrieve unique identifier. Resource Location is not an identifier.");
		}

		return get<location_class_id_type::system_guid>(path_or_id_);
	}

	std::optional<resource_location> resource_location::from_string(const string_type& text)
	{
		if (text.starts_with(path_location_prefix_))
		{
			return resource_location(text.substr(path_location_prefix_.size()));
		}

		if (text.starts_with(guid_location_prefix_))
		{
			const auto id(guid_type::from_string(text.substr(guid_location_prefix_.size())));
			if (!id.has_value())
			{
				return {};
			}

			return resource_location(id.value());
		}

		return {};
	}

	resource_location::string_type resource_location::to_string() const
	{
		switch (path_or_id_.index())
		{
		case location_class_id_type::empty:
			return {};

		case location_class_id_type::system_path:
			return path_location_prefix_ + get<location_class_id_type::system_path>(path_or_id_).string();

		case location_class_id_type::system_guid:
			return guid_location_prefix_ + get<location_class_id_type::system_guid>(path_or_id_).to_string();
		}

		return {};
	}


}
