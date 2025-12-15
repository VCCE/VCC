#pragma once
#include "vcc/utils/basic_guid.h"
#include <vcc/detail/exports.h>
#include <filesystem>
#include <variant>
#include <optional>


namespace vcc::utils
{

	/// @brief Defines a simple variant container for transporting cartridge paths.
	class cartridge_path
	{
	private:

		/// @brief Cartridge type classifications.
		/// 
		/// The order of items is important here as the value_type uses them to
		/// determine the cartridge type by the index of the variant.
		enum cartridge_class_id
		{
			/// @brief The cartridge path is empty.
			empty,
			/// @brief The cartridge path is for a ROM Pak Cartridge.
			rom,
			/// @brief The cartridge path is for a Device Cartridge.
			device
		};


	public:

		/// @brief Type alias for variable length strings.
		using string_type = std::string;
		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;
		/// @brief Type alias for a globally unique identifier.
		using guid_type = ::vcc::utils::basic_guid;
		/// @brief Type alias for cartridge type classifications.
		using cartridge_class_id_type = cartridge_class_id;
		/// @brief Type alias for the variant that will be used to store the various path
		/// types.
		/// 
		/// The order of types is important here as their index is used to determine
		/// their type based on `cartridge_class_id_type`.
		using value_type = std::variant<std::monostate, path_type, guid_type>;


	public:

		LIBCOMMON_EXPORT cartridge_path() = default;

		/// @brief Constructs a Cartridge Path to a ROM-Pak Cartridge.
		/// 
		/// @param path The system path to the cartridge file.
		/// 
		/// @throws std::invalid_argument If path is empty.
		LIBCOMMON_EXPORT cartridge_path(path_type path);

		/// @brief Constructs a Cartridge Path to a Device Cartridge.
		/// 
		/// @param id The identifier of the Device Cartridge.
		LIBCOMMON_EXPORT cartridge_path(guid_type id);

		/// @brief Determines if the cartridge path is for a ROM Pak Cartridge.
		/// 
		/// @return `true` if the path is for a ROM Pak Cartridge; `false` otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT bool is_rom_cartridge() const noexcept;

		/// @brief Determines if the cartridge path is for a Device Cartridge.
		/// 
		/// @return `true` if the path is for a Device Cartridge; `false` otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT bool is_device_cartridge() const noexcept;

		/// @brief Retrieve the path to a ROM Pak Cartridge.
		/// 
		/// @return The path to a ROM Pak Cartridge.
		/// 
		/// @throws std::runtime_error If the path is not to a ROM Pak Cartridge.
		[[nodiscard]] LIBCOMMON_EXPORT path_type rom_path() const;

		/// @brief Retrieve the path to a Device Cartridge.
		/// 
		/// @return The path to a Device Cartridge.
		/// 
		/// @throws std::runtime_error If the path is not to a Device Cartridge.
		[[nodiscard]] LIBCOMMON_EXPORT guid_type device_id() const;

		/// @brief Creates a Cartridge Path from a string.
		/// 
		/// Creates a Cartridge Path from a specially formatted string. The function will
		/// parse the string looking for prefixes and other details that specify the type
		/// of cartridge the path is for, extract the path information, and create a
		/// Cartridge path. If the type of path cannot be determined and the conversion 
		/// fails an empty value is returned.
		/// 
		/// @param text The path text to create the Cartridge Path from.
		/// 
		/// @return A cartridge path or an empty value if the path could not be converted.
		[[nodiscard]] LIBCOMMON_EXPORT static std::optional<cartridge_path> from_string(const string_type& text);

		/// @brief Convert the cartridge path to a plain text string.
		/// 
		/// Convert the cartridge path to a plain text string that can stored in a
		/// configuration file or used in diagnostic logs.
		/// 
		/// @return A string representing the path.
		[[nodiscard]] LIBCOMMON_EXPORT string_type to_string() const;


	private:

		/// @brief The path.
		value_type path_or_id_;
	};

}
