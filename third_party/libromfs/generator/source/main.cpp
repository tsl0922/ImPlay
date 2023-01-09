#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

    std::string replace(std::string string, const std::string &from, const std::string &to) {
        if(!from.empty())
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

}

int main() {
    std::ofstream outputFile("libromfs_resources.cpp");

    std::printf("libromfs: Resource Folder: %s\n", RESOURCE_LOCATION);

    outputFile << "#include <romfs/romfs.hpp>\n\n";
    outputFile << "#include <map>\n";
    outputFile << "#include <string>\n";
    outputFile << "#include <filesystem>\n";
    outputFile << "#include <vector>\n";

    outputFile << "\n\n";
    outputFile << "/* Resource definitions */\n";

    std::vector<std::filesystem::path> paths;
    std::uint64_t identifierCount = 0;
    for (const auto &entry : fs::recursive_directory_iterator(RESOURCE_LOCATION)) {
        if (!entry.is_regular_file()) continue;

        auto path = fs::canonical(fs::absolute(entry.path()));
        auto relativePath = fs::relative(entry.path(), fs::absolute(RESOURCE_LOCATION));

        outputFile << "static std::array<uint8_t, " << entry.file_size() + 1 << "> " << "resource_" LIBROMFS_PROJECT_NAME "_" << identifierCount << " = {\n";
        outputFile << "    ";

        std::vector<std::byte> bytes;
        bytes.resize(entry.file_size());

        auto file = std::fopen(entry.path().string().c_str(), "rb");
        bytes.resize(std::fread(bytes.data(), 1, entry.file_size(), file));
        std::fclose(file);

        outputFile << std::hex << std::uppercase << std::setfill('0') << std::setw(2);
        for (std::byte byte : bytes) {
            outputFile << "0x" << static_cast<std::uint32_t>(byte) << ", ";
        }
        outputFile << std::dec << std::nouppercase << std::setfill(' ') << std::setw(0);

        outputFile << "\n 0x00 };\n\n";

        paths.push_back(relativePath);

        identifierCount++;
    }

    outputFile << "\n";

    {
        outputFile << "/* Resource map */\n";
        outputFile << "const std::map<std::filesystem::path, romfs::Resource>& RomFs_" LIBROMFS_PROJECT_NAME "_get_resources() {\n";
        outputFile << "    static std::map<std::filesystem::path, romfs::Resource> resources = {\n";

        for (std::uint64_t i = 0; i < identifierCount; i++) {
            std::printf("libromfs: Bundling resource: %s\n", paths[i].string().c_str());

            outputFile << "        " << "{ \"" << toPathString(paths[i].string()) << "\", romfs::Resource({ reinterpret_cast<std::byte*>(resource_" LIBROMFS_PROJECT_NAME "_" << i << ".data()), " << "resource_" LIBROMFS_PROJECT_NAME "_" << i << ".size() - 1 }) " << "},\n";
        }
        outputFile << "    };";

        outputFile << "\n\n    return resources;\n";
        outputFile << "}\n\n";
    }

    outputFile << "\n\n";

    {
        outputFile << "/* Resource paths */\n";
        outputFile << "const std::vector<std::filesystem::path>& RomFs_" LIBROMFS_PROJECT_NAME "_get_paths() {\n";
        outputFile << "    static std::vector<std::filesystem::path> paths = {\n";

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
        outputFile << "const std::string& RomFs_" LIBROMFS_PROJECT_NAME "_get_name() {\n";
        outputFile << "    static std::string name = \"" LIBROMFS_PROJECT_NAME "\";\n";
        outputFile << "    return name;\n";
        outputFile << "}\n\n";
    }

    outputFile << "\n\n";
}
