#pragma once
#include <vcc/detail/exports.h>
#include <vcc/media/disk_image.h>
#include <memory>
#include <filesystem>


namespace vcc::utils
{

	[[nodiscard]] LIBCOMMON_EXPORT std::unique_ptr<::vcc::media::disk_image> load_disk_image(
		const std::filesystem::path& file_path);

}

