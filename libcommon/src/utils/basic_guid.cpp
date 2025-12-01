#include <vcc/utils/basic_guid.h>
#include <format>


namespace vcc::utils
{

	basic_guid::basic_guid(const guid_type& id)
		: id_(id)
	{}

	basic_guid::string_type basic_guid::to_string() const
	{
		std::string text;

		for(const auto& value : id_)
		{
			text += std::format("{:02x}", value);
		}

		return text;
	}

	std::optional<basic_guid> basic_guid::from_string(const string_type& text)
	{
		try
		{
			guid_type id;

			if (text.size() != id.size() * 2)
			{
				return {};
			}

			for (auto index(0u); index < id.size(); index++)
			{
				id[index] = static_cast<guid_type::value_type>(std::stoul(text.substr(index * 2, 2), nullptr, 16));
			}

			return basic_guid(id);
		}
		catch (std::invalid_argument&)
		{
			return {};
		}
		catch (std::out_of_range&)
		{
			return {};
		}

		return {};
	}

}
