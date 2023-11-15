#include <romfs/romfs.hpp>

std::span<romfs::impl::ResourceLocation> ROMFS_CONCAT(ROMFS_NAME, _get_resources)();
std::span<std::string_view> ROMFS_CONCAT(ROMFS_NAME, _get_paths)();
const char* ROMFS_CONCAT(ROMFS_NAME, _get_name)();

namespace romfs {

    const romfs::Resource &impl::ROMFS_CONCAT(get_, LIBROMFS_PROJECT_NAME)(const std::filesystem::path &path) {
        for (const auto &[resourcePath, resourceData] : ROMFS_CONCAT(ROMFS_NAME, _get_resources)()) {
            if (path == resourcePath)
                return resourceData;
        }

        throw std::invalid_argument(std::string("Invalid romfs resource path! File '") + std::string(romfs::name()) + "' : " + path.string());
    }

    std::vector<std::filesystem::path> impl::ROMFS_CONCAT(list_, LIBROMFS_PROJECT_NAME)(const std::filesystem::path &parent) {
        std::vector<std::filesystem::path> result;
        for (const auto &pathString : ROMFS_CONCAT(ROMFS_NAME, _get_paths)()) {
            auto path = std::filesystem::path(pathString);
            if (path.parent_path() == parent)
                result.emplace_back(std::move(path));
        }

        return result;
    }

    std::string_view impl::ROMFS_CONCAT(name_, LIBROMFS_PROJECT_NAME)() {
        return ROMFS_CONCAT(ROMFS_NAME, _get_name)();
    }

}