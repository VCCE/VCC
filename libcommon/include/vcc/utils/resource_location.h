#pragma once
#include "vcc/utils/basic_guid.h"
#include "vcc/utils/detail/location_class_id.h"
#include <vcc/detail/exports.h>
#include <filesystem>
#include <variant>
#include <optional>


namespace vcc::utils
{

	/// @brief Defines a simple variant container for locating resources.
	class resource_location
	{
	private:


	public:

		/// @brief Type alias for variable length strings.
		using string_type = std::string;
		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;
		/// @brief Type alias for a globally unique identifier.
		using guid_type = ::vcc::utils::basic_guid;
		/// @brief Type alias for resource location type classifications.
		using location_class_id_type = ::vcc::utils::detail::location_class_id;
		/// @brief Type alias for the variant that will be used to store the various path
		/// types.
		/// 
		/// The order of types is important here as their index is used to determine
		/// their type based on `location_class_id_type`.
		using value_type = std::variant<std::monostate, path_type, guid_type>;


	public:

		LIBCOMMON_EXPORT resource_location() = default;

		/// @brief Constructs a Resource Location to a file path.
		/// 
		/// @param path The system path to the file.
		/// 
		/// @throws std::invalid_argument If `path` is empty.
		LIBCOMMON_EXPORT resource_location(path_type path);

		/// @brief Constructs a Resource Location defined by a unique identifier.
		/// 
		/// @param id The identifier of the location.
		LIBCOMMON_EXPORT resource_location(guid_type id);

		/// @brief Determines if the location is empty.
		/// 
		/// @return `true` if the path location is empty; `false` otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT bool empty() const noexcept;

		/// @brief Determines if the location is a file path.
		/// 
		/// @return `true` if the location is a file path; `false` otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT bool is_path() const noexcept;

		/// @brief Determines if location is a unique identifier.
		/// 
		/// @return `true` if the path is a unique identifier; `false` otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT bool is_guid() const noexcept;

		/// @brief Retrieve the location path.
		/// 
		/// @return The path to a file.
		/// 
		/// @throws std::runtime_error If the path is location is not a path.
		[[nodiscard]] LIBCOMMON_EXPORT path_type path() const;

		/// @brief Retrieve the location is a unique identifier.
		/// 
		/// @return The path a unique identifier.
		/// 
		/// @throws std::runtime_error If the path is not a unique identifier.
		[[nodiscard]] LIBCOMMON_EXPORT guid_type guid() const;

		/// @brief Creates a Resource Location from a string.
		/// 
		/// Creates a Resource Location from a specially formatted string. The function
		/// will parse the string looking for prefixes and other details that specify the
		/// type and location of a resource, extract the path information, and create a
		/// Resource Location. If the type of location cannot be determined and the
		/// conversion fails an empty value is returned.
		/// 
		/// @param text The path text to create the Resource Location from.
		/// 
		/// @return A Resource Location or an empty value if the location could not be converted.
		[[nodiscard]] LIBCOMMON_EXPORT static std::optional<resource_location> from_string(const string_type& text);

		/// @brief Convert the Resource Location to a plain text string.
		/// 
		/// Convert the Resource Location to a plain text string that can stored in a
		/// configuration file or used in diagnostic logs.
		/// 
		/// @return A string representing the path.
		[[nodiscard]] LIBCOMMON_EXPORT string_type to_string() const;


	private:

		/// @brief Prefix for locations that are a system path.
		static const inline string_type path_location_prefix_ = "file:";
		/// @brief Prefix for locations that are a unique identifier.
		static const inline string_type guid_location_prefix_ = "guid:";


	private:

		/// @brief The path.
		value_type path_or_id_;
	};

}
