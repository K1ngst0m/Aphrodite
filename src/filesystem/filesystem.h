#pragma once

#include "common/hash.h"
#include "common/logger.h"
#include "common/result.h"

namespace aph
{
class Filesystem final
{
public:
    // Constructors and Destructors
    Filesystem()                             = default;
    Filesystem(const Filesystem&)            = delete;
    Filesystem(Filesystem&&)                 = delete;
    Filesystem& operator=(const Filesystem&) = delete;
    Filesystem& operator=(Filesystem&&)      = delete;
    ~Filesystem();

    // File Path Operations
    auto resolvePath(std::string_view inputPath) const -> Expected<std::string>;
    auto getCurrentWorkingDirectory() const -> std::string;
    auto absolutePath(std::string_view inputPath) const -> std::string;
    auto getFileExtension(std::string_view path) const -> std::string;

    // File System Operations
    auto exist(std::string_view path) const -> bool;
    auto createDirectories(std::string_view path) const -> Result;
    auto getLastModifiedTime(std::string_view path) const -> uint64_t;
    auto getFileSize(std::string_view path) const -> size_t;

    // File Reading Operations
    auto readFileToString(std::string_view path) const -> Expected<std::string>;
    auto readFileToBytes(std::string_view path) const -> Expected<std::vector<uint8_t>>;
    auto readFileLines(std::string_view path) const -> Expected<std::vector<std::string>>;
    template <typename T>
    auto readBinaryData(std::string_view path, T* data, size_t count) const -> Expected<bool>;

    // File Writing Operations
    auto writeStringToFile(std::string_view path, const std::string& content) const -> Result;
    auto writeBytesToFile(std::string_view path, const std::vector<uint8_t>& bytes) const -> Result;
    auto writeLinesToFile(std::string_view path, const std::vector<std::string>& lines) const -> Result;
    template <typename T>
    auto writeBinaryData(std::string_view path, const T* data, size_t count) const -> Result;

    // Memory Mapping Operations
    auto map(std::string_view path) -> void*;
    void unmap(void* data);
    void clearMappedFiles();

    // Protocol Management
    void registerProtocol(auto&& protocols);
    void registerProtocol(const std::string& protocol, const std::string& path);
    auto protocolExists(const std::string& protocol) const -> bool;
    void removeProtocol(const std::string& protocol);

private:
    HashMap<int, std::function<void()>> m_callbacks;
    HashMap<std::string, std::string> m_protocols;
    HashMap<void*, std::size_t> m_mappedFiles;
    std::mutex m_mapLock;
};

template <typename T>
inline auto Filesystem::readBinaryData(std::string_view path, T* data, size_t count) const -> Expected<bool>
{
    if (!exist(path))
        return Expected<bool>{ Result::RuntimeError, "File not found: " + std::string(path) };

    if (!data || count == 0)
        return Expected<bool>{ Result::ArgumentOutOfRange, "Invalid data pointer or count" };

    auto bytes = readFileToBytes(path);
    if (!bytes.success())
        return Expected<bool>{ bytes.error() };

    if (bytes.value().size() < sizeof(T) * count)
        return Expected<bool>{ Result::RuntimeError, "File size too small, expected at least " +
                                                         std::to_string(sizeof(T) * count) + " bytes but got " +
                                                         std::to_string(bytes.value().size()) };

    std::memcpy(data, bytes.value().data(), sizeof(T) * count);
    return true;
}

template <typename T>
inline auto Filesystem::writeBinaryData(std::string_view path, const T* data, size_t count) const -> Result
{
    if (!data || count == 0)
        return { Result::ArgumentOutOfRange, "Invalid data pointer or count" };

    std::vector<uint8_t> bytes(sizeof(T) * count);
    std::memcpy(bytes.data(), data, bytes.size());

    return writeBytesToFile(path, bytes);
}

inline void Filesystem::registerProtocol(auto&& protocols)
{
    m_protocols = APH_FWD(protocols);
}

} // namespace aph
