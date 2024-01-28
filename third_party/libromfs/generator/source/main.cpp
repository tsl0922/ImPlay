#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <ranges>
#include <string_view>

namespace fs = std::filesystem;

namespace {

    std::string replace(std::string string, const std::string &from, const std::string &to) {
        if (!from.empty())
            for(size_t pos = 0; (pos = string.find(from, pos)) != std::string::npos; pos += to.size())
                string.replace(pos, from.size(), to);
        return string;
    }

    std::string toPathString(std::string string) {
        // Replace all backslashes with forward slashes on Windows
        #if defined (_WIN32)
            string = replace(string, "\\", "/");
        #endif

        // Replace all " with \"
        string = replace(string, "\"", "\\\"");

        return string;
    }

    std::vector<std::string> splitString(std::string string, const std::string &delimiter){
        std::vector<std::string> result;

        size_t position = 0;
        const size_t size = string.size();

        while (position < size) {
            position = string.find(delimiter);
            result.push_back(string.substr(0, position));
            string.erase(0, position + delimiter.size());
        }

        return result;
    }

}

int main() {
    std::ofstream outputFile("libromfs_resources.cpp");

    std::printf("[libromfs] Resource Folders: %s\n", RESOURCE_LOCATION);

    outputFile << "#include <romfs/romfs.hpp>\n\n";
    outputFile << "#include <array>\n";
    outputFile << "#include <cstdint>\n";

    outputFile << "\n\n";
    outputFile << "/* Resource definitions */\n";

    std::vector<std::filesystem::path> paths;
    std::uint64_t identifierCount = 0;

    auto resourceLocations = splitString(RESOURCE_LOCATION, ",");
    for (const auto &resourceLocation : resourceLocations) {
        std::printf("[libromfs] Packing resource folder: %s\n", resourceLocation.c_str());

        std::error_code errorCode;
        if (!std::filesystem::exists(resourceLocation, errorCode)) {
            if (errorCode) {
                std::printf("[libromfs] Error: %s\n", errorCode.message().c_str());
            }

            continue;
        }

        outputFile << "\n/* Resource folder: " << resourceLocation << " */\n";
        for (const auto &entry : fs::recursive_directory_iterator(resourceLocation)) {
            if (!entry.is_regular_file()) continue;

            auto path = fs::canonical(fs::absolute(entry.path()));
            auto relativePath = fs::relative(entry.path(), fs::absolute(resourceLocation));

            outputFile << "static std::array<std::uint8_t, " << entry.file_size() + 1 << "> " << "resource_" LIBROMFS_PROJECT_NAME "_" << identifierCount << " = {\n";
            outputFile << "    ";

            std::vector<std::byte> bytes;
            bytes.resize(entry.file_size());

            auto file = std::fopen(entry.path().string().c_str(), "rb");
            bytes.resize(std::fread(bytes.data(), 1, entry.file_size(), file));
            std::fclose(file);

            for (std::byte byte : bytes) {
                outputFile << static_cast<std::uint32_t>(byte) << ",";
            }

            outputFile << "0 };\n\n";

            paths.push_back(relativePath);

            identifierCount++;
        }
    }

    outputFile << "\n";

    {
        outputFile << "/* Resource map */\n";
        outputFile << "ROMFS_VISIBILITY std::span<romfs::impl::ResourceLocation> RomFs_" LIBROMFS_PROJECT_NAME "_get_resources() {\n";
        outputFile << "    static std::array<romfs::impl::ResourceLocation, " << identifierCount << "> resources = {\n";

        for (std::uint64_t i = 0; i < identifierCount; i++) {
            std::printf("[libromfs] Bundling resource: %s\n", paths[i].string().c_str());

            outputFile << "        " << "romfs::impl::ResourceLocation { \"" << toPathString(paths[i].string()) << "\", romfs::Resource({ reinterpret_cast<std::byte*>(resource_" LIBROMFS_PROJECT_NAME "_" << i << ".data()), " << "resource_" LIBROMFS_PROJECT_NAME "_" << i << ".size() - 1 }) " << "},\n";
        }
        outputFile << "    };";

        outputFile << "\n\n    return resources;\n";
        outputFile << "}\n\n";
    }

    outputFile << "\n\n";

    {
        outputFile << "/* Resource paths */\n";
        outputFile << "ROMFS_VISIBILITY std::span<std::string_view> RomFs_" LIBROMFS_PROJECT_NAME "_get_paths() {\n";
        outputFile << "    static std::array<std::string_view, " << identifierCount << "> paths = {\n";

        for (std::uint64_t i = 0; i < identifierCount; i++) {
            outputFile << "        \"" << toPathString(paths[i].string()) << "\",\n";
        }
        outputFile << "    };";

        outputFile << "\n\n    return paths;\n";
        outputFile << "}\n\n";
    }

    outputFile << "\n\n";

    {
        outputFile << "/* RomFS name */\n";
        outputFile << "ROMFS_VISIBILITY const char* RomFs_" LIBROMFS_PROJECT_NAME "_get_name() {\n";
        outputFile << "    return \"" LIBROMFS_PROJECT_NAME "\";\n";
        outputFile << "}\n\n";
    }

    outputFile << "\n\n";
}
