#pragma once

#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <string_view>
#include <span>
#include <vector>

#define ROMFS_CONCAT_IMPL(x, y) x##y
#define ROMFS_CONCAT(x, y) ROMFS_CONCAT_IMPL(x, y)

#define ROMFS_NAME ROMFS_CONCAT(RomFs_, LIBROMFS_PROJECT_NAME)

#if !defined(WIN32)
    #define ROMFS_VISIBILITY [[gnu::visibility("hidden")]]
#else
    #define ROMFS_VISIBILITY
#endif

template<typename T>
concept ByteType = std::is_trivial_v<T> && sizeof(T) == sizeof(std::byte);

namespace romfs {

    class Resource {
    public:
        constexpr Resource() = default;
        explicit constexpr Resource(const std::span<std::byte> &content) : m_content(content) {}

        template<ByteType T = std::byte>
        [[nodiscard]] constexpr const T* data() const {
            return reinterpret_cast<const T*>(this->m_content.data());
        }

        template<ByteType T = std::byte>
        [[nodiscard]] constexpr std::span<const T> span() const {
            return { this->data<T>(), this->size() };
        }

        [[nodiscard]]
        constexpr std::size_t size() const {
            return this->m_content.size();
        }

        [[nodiscard]]
        std::string_view string() const {
            return { reinterpret_cast<const char*>(this->data()), this->size() + 1 };
        }

        [[nodiscard]]
        std::u8string_view u8string() const {
            return { reinterpret_cast<const char8_t *>(this->data()), this->size() + 1 };
        }

        [[nodiscard]]
        constexpr bool valid() const {
            return !this->m_content.empty() && this->m_content.data() != nullptr;
        }

    private:
        std::span<const std::byte> m_content;
    };

    namespace impl {

        struct ResourceLocation {
            std::string_view path;
            Resource resource;
        };

        [[nodiscard]] ROMFS_VISIBILITY const Resource& ROMFS_CONCAT(get_, LIBROMFS_PROJECT_NAME)(const std::filesystem::path &path);
        [[nodiscard]] ROMFS_VISIBILITY std::vector<std::filesystem::path> ROMFS_CONCAT(list_, LIBROMFS_PROJECT_NAME)(const std::filesystem::path &path);
        [[nodiscard]] ROMFS_VISIBILITY std::string_view ROMFS_CONCAT(name_, LIBROMFS_PROJECT_NAME)();

    }

    [[nodiscard]] ROMFS_VISIBILITY inline const Resource& get(const std::filesystem::path &path) { return impl::ROMFS_CONCAT(get_, LIBROMFS_PROJECT_NAME)(path); }
    [[nodiscard]] ROMFS_VISIBILITY inline std::vector<std::filesystem::path> list(const std::filesystem::path &path = {}) { return impl::ROMFS_CONCAT(list_, LIBROMFS_PROJECT_NAME)(path); }
    [[nodiscard]] ROMFS_VISIBILITY inline std::string_view name() { return impl::ROMFS_CONCAT(name_, LIBROMFS_PROJECT_NAME)(); }


}