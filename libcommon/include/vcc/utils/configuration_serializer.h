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

#pragma once
#include <vcc/core/detail/exports.h>
#include <string>

// The configuration_serializer class reads or writes VCC settings kept in the
// VCC initialization file (typically vcc.ini)
//
// In programming, a serializer is a mechanism or tool used to convert a complex
// data structure or object into a format that can be easily stored, transmitted,
// or reconstructed later. This class barely meets that definition. It might be
// better named "settings_manager" or just "Settings".

namespace vcc::utils
{

	/// @brief Provides facilities to save and restore values.
	///
	/// The Configuration Serializer provides facilities for saving values to and loading
	/// values from a file that persists between sessions. Values are stored grouped in
	/// sections and are accessed using a textual key.
	class configuration_serializer
	{
	public:

		/// @brief The type used to represent paths.
		using path_type = ::std::string;
		/// @brief The type used to represent strings.
		using string_type = ::std::string;
		/// @brief The type used to represent a size or length.
		using size_type = ::std::size_t;

	public:

		/// @brief Constructs a Configuration Serializer.
		/// 
		/// @param path The path to the file where the values are stored.
		LIBCOMMON_EXPORT explicit configuration_serializer(path_type path);

		/// @brief Save a signed integer value.
		/// 
		/// @param section The section to store the value in.
		/// @param key The key used to identify the value in.
		/// @param value The integer value to store.
		LIBCOMMON_EXPORT [[nodiscard]] void write(
			const string_type& section,
			const string_type& key,
			int value) const;

		/// @brief Save a string value.
		/// 
		/// @param section The section to store the value in.
		/// @param key The key used to identify the value in.
		/// @param value The string value to store.
		LIBCOMMON_EXPORT [[nodiscard]] void write(
			const string_type& section,
			const string_type& key,
			const string_type& value) const;

		/// @brief Retrieve a boolean value.
		/// 
		/// Retrieves a boolean value from the configuration. If the value is not present in
		/// the configuration, a default value is returned.
		/// 
		/// @param section The section the value is stored in.
		/// @param key The key the value is saved as.
		/// @param default_value The value to return if it does not exist in the configuration.
		/// 
		/// @return The value stored in the configuration if it exists; otherwise the specified
		/// default value.
		LIBCOMMON_EXPORT [[nodiscard]] bool read(
			const string_type& section,
			const string_type& key,
			bool default_value) const;

		/// @brief Retrieve a signed integer value.
		/// 
		/// Retrieves a signed integer value from the configuration. If the value is not present
		/// in the configuration, a default value is returned.
		/// 
		/// @param section The section the value is stored in.
		/// @param key The key the value is saved as.
		/// @param default_value The value to return if it does not exist in the configuration.
		/// 
		/// @return The value stored in the configuration if it exists; otherwise the specified
		/// default value.
		LIBCOMMON_EXPORT [[nodiscard]] int read(
			const string_type& section,
			const string_type& key,
			const int& default_value) const;

		/// @brief Retrieve an unsigned integer value.
		/// 
		/// Retrieves an unsigned integer value from the configuration. If the value is not present
		/// in the configuration, a default value is returned.
		/// 
		/// @param section The section the value is stored in.
		/// @param key The key the value is saved as.
		/// @param default_value The value to return if it does not exist in the configuration.
		/// 
		/// @return The value stored in the configuration if it exists; otherwise the specified
		/// default value.
		LIBCOMMON_EXPORT [[nodiscard]] size_type read(
			const string_type& section,
			const string_type& key,
			const size_type& default_value) const;

		/// @brief Retrieve a string value.
		/// 
		/// Retrieves a string value from the configuration. If the value is not present in the
		/// configuration, a default value is returned.
		/// 
		/// @param section The section the value is stored in.
		/// @param key The key the value is saved as.
		/// @param default_value The value to return if it does not exist in the configuration.
		/// 
		/// @return The value stored in the configuration if it exists; otherwise the specified
		/// default value.
		LIBCOMMON_EXPORT [[nodiscard]] string_type read(
			const string_type& section,
			const string_type& key,
			const string_type& default_value = {}) const;

		/// @brief Retrieve a string value.
		/// 
		/// Retrieves a string value from the configuration. If the value is not present in the
		/// configuration, a default value is returned.
		/// 
		/// @param section The section the value is stored in.
		/// @param key The key the value is saved as.
		/// @param default_value The value to return if it does not exist in the configuration.
		/// 
		/// @return The value stored in the configuration if it exists; otherwise the specified
		/// default value.
		LIBCOMMON_EXPORT [[nodiscard]] string_type read(
			const string_type& section,
			const string_type& key,
			const char* default_value) const;


	private:

		const path_type path_;
	};
}
