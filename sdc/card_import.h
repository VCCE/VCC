#pragma once
#include <filesystem>
#include <string>
namespace sdc_card {
// Creates a new card beneath storage. On failure, removes only that new card.
// The ZIP is opened read-only and never changed.
std::filesystem::path import_zip(const std::filesystem::path& zip,
                                 const std::filesystem::path& storage);
}
