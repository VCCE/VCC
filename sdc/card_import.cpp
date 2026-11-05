#include "card_import.h"
#include "../3rdparty/miniz/miniz.h"
#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>
#include <vector>
#include <chrono>
#include <Windows.h>

namespace sdc_card 
{
    namespace fs = std::filesystem;

    static std::string lower(std::string s) { for (char& c : s) c = (char)std::tolower((unsigned char)c); return s; }

    static fs::path entry_path(std::string name) 
    {
        std::replace(name.begin(), name.end(), '\\', '/');
        if (name.empty() || name[0] == '/' || name.find('\0') != std::string::npos)
            throw std::runtime_error("ZIP contains an invalid or absolute path");
        fs::path result;
        size_t start = 0;
        while (start < name.size()) 
        {
            auto end = name.find('/', start);
            if (end == std::string::npos)
                end = name.size();
            std::string part = name.substr(start, end - start);
            start = end + 1;
            if (part.empty() || part == "." || part == ".." || part.back() == '.' || part.back() == ' ')
                throw std::runtime_error("ZIP contains an unsafe path component");
            for (unsigned char c : part)
                if (c < 32 || std::string(":<>\"|?*").find(c) != std::string::npos)
                    throw std::runtime_error("ZIP contains a filename Windows cannot use");
            std::string device = lower(part.substr(0, part.find('.')));
            if (device == "con" || device == "prn" || device == "aux" || device == "nul" ||
                (device.size() == 4 && (device.substr(0, 3) == "com" || device.substr(0, 3) == "lpt") && 
                    device[3] >= '0' && device[3] <= '9'))
                throw std::runtime_error("ZIP contains a reserved Windows filename");
            result /= fs::u8path(part);
        }
        return result;
    }

    std::string cp437_to_utf8(const char* cp437_str, int len) 
    {
        // CP437 -> UTF-16
        int wide_len = MultiByteToWideChar(437, 0, cp437_str, len, nullptr, 0);
        std::wstring wide(wide_len, L'\0');
        MultiByteToWideChar(437, 0, cp437_str, len, &wide[0], wide_len);

        // UTF-16 -> UTF-8
        int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide_len, nullptr, 0, nullptr, nullptr);
        std::string utf8(utf8_len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide_len, &utf8[0], utf8_len, nullptr, nullptr);
        return utf8;
    }

    fs::path import_zip(const fs::path& input, const fs::path& storage) 
    {
        mz_zip_archive zip = {};
        if (!mz_zip_reader_init_file(&zip, input.string().c_str(), 0))
            throw std::runtime_error("Cannot open ZIP archive");
        struct Close { mz_zip_archive* z; ~Close() { mz_zip_reader_end(z); } } close{ &zip };
        fs::path card;
        try 
        {
            struct Entry { mz_uint index; fs::path path; bool directory; };
            std::vector<Entry> entries;
            std::set<std::string> names;
            mz_uint count = mz_zip_reader_get_num_files(&zip);
            if (count == 0 || count > 100000)
                throw std::runtime_error("ZIP is empty or has too many entries");
            uint64_t total = 0;
            for (mz_uint i = 0;i < count;i++) 
            {
                mz_zip_archive_file_stat info;
                if (!mz_zip_reader_file_stat(&zip, i, &info) || info.m_is_encrypted || !info.m_is_supported)
                    throw std::runtime_error("ZIP has an encrypted or unsupported entry");
                if (((info.m_external_attr >> 16) & 0170000) == 0120000)
                    throw std::runtime_error("ZIP symlinks are not supported");
                auto size = mz_zip_reader_get_filename(&zip, i, nullptr, 0);
                if (size == 0 || size > 1024)
                    throw std::runtime_error("ZIP filename is too long");
                std::vector<char> name(size);
                mz_zip_reader_get_filename(&zip, i, name.data(), size);
                if (std::string(name.data()).size() + 1 != size)
                    throw std::runtime_error("ZIP filename contains a null byte");
                fs::path relative = entry_path(cp437_to_utf8(name.data(), size));
                if (relative.generic_string().size() > 180)
                    throw std::runtime_error("ZIP path exceeds plugin path limit");
                if (!names.insert(lower(relative.generic_string())).second)
                    throw std::runtime_error("ZIP contains duplicate filenames");
                if (info.m_uncomp_size > 8ULL * 1024 * 1024 * 1024 - total)
                    throw std::runtime_error("ZIP exceeds 8 GiB import limit");
                total += info.m_uncomp_size;
                entries.push_back({ i,relative,info.m_is_directory != 0 });
            }
            fs::create_directories(storage);
            auto stamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            for (int attempt = 0;attempt < 1000;attempt++) 
            {
                auto candidate = storage / ("Card-" + stamp + "-" + std::to_string(attempt));
                if (candidate.string().size() > 230)
                    throw std::runtime_error("SD card storage path is too long");
                if (fs::create_directory(candidate)) 
                { 
                    card = candidate;
                    break; 
                }
            }
            if (card.empty())
                throw std::runtime_error("Cannot allocate a new SD card folder");
            for (auto& entry : entries) 
            {
                fs::path output = card / entry.path;
                if (output.string().size() >= 260)
                    throw std::runtime_error("Extracted path exceeds Windows plugin limit");
                if (entry.directory) 
                { 
                    fs::create_directories(output);
                    continue; 
                }
                fs::create_directories(output.parent_path());
                if (!mz_zip_reader_extract_to_file(&zip, entry.index, output.string().c_str(), 0))
                    throw std::runtime_error("ZIP extraction failed (corrupt data, CRC, disk space or access denied)");
            }
            // Accommodate archives with a single enclosing SD-card folder.
            fs::path root = card;
            for (;;) 
            {
                if (fs::exists(root / "STARTUP.CFG") || fs::exists(root / "startup.cfg")) break;
                std::vector<fs::path> children;
                for (auto& child : fs::directory_iterator(root))
                    children.push_back(child.path());
                if (children.size() != 1 || !fs::is_directory(children[0])) break;
                root = children[0];
            }
            return root;
        }
        catch (...) 
        { 
            if (!card.empty()) 
            { 
                std::error_code ignored;
                fs::remove_all(card, ignored); 
            }
            throw; 
        }
    }
}
