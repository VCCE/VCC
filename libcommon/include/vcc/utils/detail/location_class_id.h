#pragma once


namespace vcc::utils::detail
{

	/// @brief Resource location classifications.
	/// 
	/// The order of items is important here as the value_type uses them to
	/// determine the location type by the index of the variant.
	enum location_class_id
	{
		/// @brief The location is empty.
		empty,
		/// @brief The location is a path.
		path,
		/// @brief The location is defined by a unique identifier.
		guid
	};

}
