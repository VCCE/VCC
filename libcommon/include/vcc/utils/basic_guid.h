#pragma once
#include <vcc/detail/exports.h>
#include <string>
#include <optional>
#include <array>


namespace vcc::utils
{

	/// @brief A basic representation of a globally unique identifier.
	///
	/// This class is a simple representation of a globally unique identifier and is used
	/// to identify different components in or used by the system.
	class basic_guid
	{
	public:

		/// @brief Type alias for variable length strings.
		using string_type = std::string;
		/// @brief Container used to store the guid data.
		using guid_type = std::array<unsigned char, 16>;


	public:

		/// @brief Construct a Basic GUID.
		/// 
		/// Construct a Basic GUID with the id set to 00000000000000000000000000000000.
		LIBCOMMON_EXPORT basic_guid() = default;

		/// @brief Construct a Basic GUID.
		/// 
		/// Construct a Basic GUID from guid data.
		LIBCOMMON_EXPORT explicit basic_guid(const guid_type& id);

		/// @brief Retrieve the GUID data.
		/// 
		/// @return A copy of the raw GUID identifier data.
		[[nodiscard]] LIBCOMMON_EXPORT guid_type id() const
		{
			return id_;
		}

		/// @brief Convert the Basic GUID to a string representation.
		/// 
		/// @return A string containing the textual representation of the GUID.
		[[nodiscard]] LIBCOMMON_EXPORT string_type to_string() const;

		/// @brief Creates a Basic GUID from a string representation.
		/// 
		/// @return A Basic GUID if the conversion is suffessful; an empty value otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT static std::optional<basic_guid> from_string(const string_type& text);

		/// @brief Compare two identifiers.
		/// 
		/// @param other The identifier to compare.
		/// 
		/// @return `true` if `this` is less than `other`; `false` otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT bool operator<(const basic_guid& other) const noexcept
		{
			return std::less<guid_type>()(id(), other.id());
		}


	private:

		/// @brief The GUID data.
		guid_type id_ = {};
	};

}
