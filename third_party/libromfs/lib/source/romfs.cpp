#include <romfs/romfs.hpp>

#include <map>

const std::map<std::filesystem::path, romfs::Resource>& ROMFS_CONCAT(ROMFS_NAME, _get_resources)();
const std::vector<std::filesystem::path>& ROMFS_CONCAT(ROMFS_NAME, _get_paths)();
const std::string& ROMFS_CONCAT(ROMFS_NAME, _get_name)();

namespace romfs {

    const romfs::Resource &impl::ROMFS_CONCAT(get_, LIBROMFS_PROJECT_NAME)(const std::filesystem::path &path) {
        return ROMFS_CONCAT(ROMFS_NAME, _get_resources)().at(path);
    }

    std::vector<std::filesystem::path> impl::ROMFS_CONCAT(list_, LIBROMFS_PROJECT_NAME)(const std::filesystem::path &parent) {
        if (parent.empty()) {
            return ROMFS_CONCAT(ROMFS_NAME, _get_paths)();
        } else {
            std::vector<std::filesystem::path> result;
            for (const auto &p : ROMFS_CONCAT(ROMFS_NAME, _get_paths)())
                if (p.parent_path() == parent)
                    result.push_back(p);

            return result;
        }
    }

    const std::string &impl::ROMFS_CONCAT(name_, LIBROMFS_PROJECT_NAME)() {
        return ROMFS_CONCAT(ROMFS_NAME, _get_name)();
    }

}