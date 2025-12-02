////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute it and/or
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
#include "vcc/detail/exports.h"
#include <filesystem>
#include <string>


namespace vcc::utils
{

	/// @brief Provides facilities to save and restore values.
	///
	/// The Persistent Value Store provides facilities for saving values to and loading
	/// values from a file that persists between sessions. Values are stored grouped in
	/// sections and are accessed using a textual key.
	class persistent_value_store
	{
	public:

		/// @brief The type used to represent paths.
		using path_type = std::filesystem::path;
		/// @brief Defines the type used to hold a variable length string.
		using string_type = std::string;
		/// @brief The type used to represent section identifiers.
		using section_type = std::string;
		/// @brief Type alias to lengths, 1 dimension sizes, and indexes.
		using size_type = std::size_t;


	public:

		/// @brief Constructs a Persistent Value Store.
		/// 
		/// @param path The path to the file where the values are stored.
		LIBCOMMON_EXPORT explicit persistent_value_store(path_type path);

		/// @brief Remove a value from a specific section.
		/// 
		/// @param section The section the value is stored in.
		/// @param key The key the value is stored as.
		LIBCOMMON_EXPORT void remove(const section_type& section, const string_type& key) const;

		/// @brief Save a signed integer value.
		/// 
		/// @param section The section to store the value in.
		/// @param key The key used to identify the value in.
		/// @param value The integer value to store.
		LIBCOMMON_EXPORT void write(
			const section_type& section,
			const string_type& key,
			int value) const;

		/// @brief Save a string value.
		/// 
		/// @param section The section to store the value in.
		/// @param key The key used to identify the value in.
		/// @param value The string value to store.
		LIBCOMMON_EXPORT void write(
			const section_type& section,
			const string_type& key,
			const string_type& value) const;

		/// @brief Save a string value.
		/// 
		/// @param section The section to store the value in.
		/// @param key The key used to identify the value in.
		/// @param value A pointer to the null terminated string to store.
		LIBCOMMON_EXPORT void write(
			const section_type& section,
			const string_type& key,
			string_type::const_pointer value) const;

		/// @brief Save a path value.
		/// 
		/// @param section The section to store the value in.
		/// @param key The key used to identify the value in.
		/// @param value The path to store.
		LIBCOMMON_EXPORT void write(
			const section_type& section,
			const string_type& key,
			const path_type& value) const;

		/// @brief Retrieve a boolean value.
		/// 
		/// Retrieves a boolean value from the value store. If the value is not present in the
		/// value store, a default value is returned.
		/// 
		/// @param section The section the value is stored in.
		/// @param key The key the value is saved as.
		/// @param default_value The value to return if it does not exist in the value store.
		/// 
		/// @return The stored value if it exists; otherwise the specified default value.
		LIBCOMMON_EXPORT [[nodiscard]] bool read(
			const section_type& section,
			const string_type& key,
			bool default_value) const;

		/// @brief Retrieve a signed integer value.
		/// 
		/// Retrieves a signed integer value from the value store. If the value is not present
		/// in the value store, a default value is returned.
		/// 
		/// @param section The section the value is stored in.
		/// @param key The key the value is saved as.
		/// @param default_value The value to return if it does not exist in the value store.
		/// 
		/// @return The stored value if it exists; otherwise the specified default value.
		LIBCOMMON_EXPORT [[nodiscard]] int read(
			const section_type& section,
			const string_type& key,
			const int& default_value) const;

		/// @brief Retrieve an unsigned integer value.
		/// 
		/// Retrieves an unsigned integer value from the value store. If the value is not present
		/// in the value store, a default value is returned.
		/// 
		/// @param section The section the value is stored in.
		/// @param key The key the value is saved as.
		/// @param default_value The value to return if it does not exist in the value store.
		/// 
		/// @return The stored value if it exists; otherwise the specified default value.
		LIBCOMMON_EXPORT [[nodiscard]] size_type read(
			const section_type& section,
			const string_type& key,
			const size_type& default_value) const;

		/// @brief Retrieve a string value.
		/// 
		/// Retrieves a string value from the value store. If the value is not present in the
		/// value store, a default value is returned.
		/// 
		/// @param section The section the value is stored in.
		/// @param key The key the value is saved as.
		/// @param default_value The value to return if it does not exist in the value store.
		/// 
		/// @return The stored value if it exists; otherwise the specified default value.
		LIBCOMMON_EXPORT [[nodiscard]] string_type read(
			const section_type& section,
			const string_type& key,
			const string_type& default_value = {}) const;

		/// @brief Retrieve a string value.
		/// 
		/// Retrieves a string value from the value store. If the value is not present in the
		/// value store, a default value is returned.
		/// 
		/// @param section The section the value is stored in.
		/// @param key The key the value is saved as.
		/// @param default_value The value to return if it does not exist in the value store.
		/// 
		/// @return The stored value if it exists; otherwise the specified default value.
		LIBCOMMON_EXPORT [[nodiscard]] string_type read(
			const section_type& section,
			const string_type& key,
			const char* default_value) const;


	private:

		/// @brief The path where the file containing the values is stored.
		/// 
		/// This value is stored as string for convenience in working with the windows API.
		const string_type path_;
	};
}
