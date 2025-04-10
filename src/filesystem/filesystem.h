#pragma once

#include "common/hash.h"
#include "common/logger.h"
#include "common/result.h"

namespace aph
{
class Filesystem final
{
public:
    Filesystem() = default;
    Filesystem(const Filesystem&)            = delete;
    Filesystem(Filesystem&&)                 = delete;
    Filesystem& operator=(const Filesystem&) = delete;
    Filesystem& operator=(Filesystem&&)      = delete;
    ~Filesystem();

    void* map(std::string_view path);
    void unmap(void* data);
    void clearMappedFiles();

    bool exist(std::string_view path) const;
    Result createDirectories(std::string_view path) const;

    Expected<std::string> readFileToString(std::string_view path) const;
    Expected<std::vector<uint8_t>> readFileToBytes(std::string_view path) const;
    Expected<std::vector<std::string>> readFileLines(std::string_view path) const;

    Result writeStringToFile(std::string_view path, const std::string& content) const;
    Result writeBytesToFile(std::string_view path, const std::vector<uint8_t>& bytes) const;
    Result writeLinesToFile(std::string_view path, const std::vector<std::string>& lines) const;

    template <typename T>
    Result writeBinaryData(std::string_view path, const T* data, size_t count) const;
    template <typename T>
    Expected<bool> readBinaryData(std::string_view path, T* data, size_t count) const;

    void registerProtocol(auto&& protocols);
    void registerProtocol(const std::string& protocol, const std::string& path);
    bool protocolExists(const std::string& protocol) const;
    void removeProtocol(const std::string& protocol);

    Expected<std::string> resolvePath(std::string_view inputPath) const;
    std::string getCurrentWorkingDirectory() const;

    std::string absolutePath(std::string_view inputPath) const;

    // Get the last modification time of a file
    int64_t getLastModifiedTime(std::string_view path) const;
    
    // Get the size of a file
    size_t getFileSize(std::string_view path) const;
    
    // Get file extension
    std::string getFileExtension(std::string_view path) const;

private:
    HashMap<int, std::function<void()>> m_callbacks;
    HashMap<std::string, std::string> m_protocols;
    HashMap<void*, std::size_t> m_mappedFiles;
    std::mutex m_mapLock;
};

template <typename T>
inline Expected<bool> Filesystem::readBinaryData(std::string_view path, T* data, size_t count) const
{
    if (!exist(path))
        return Expected<bool>{Result::RuntimeError, "File not found: " + std::string(path)};
    
    if (!data || count == 0)
        return Expected<bool>{Result::ArgumentOutOfRange, "Invalid data pointer or count"};

    auto bytes = readFileToBytes(path);
    if (!bytes.success())
        return Expected<bool>{bytes.error()};
        
    if (bytes.value().size() < sizeof(T) * count)
        return Expected<bool>{Result::RuntimeError, 
            "File size too small, expected at least " + std::to_string(sizeof(T) * count) + 
            " bytes but got " + std::to_string(bytes.value().size())};

    std::memcpy(data, bytes.value().data(), sizeof(T) * count);
    return true;
}

template <typename T>
inline Result Filesystem::writeBinaryData(std::string_view path, const T* data, size_t count) const
{
    if (!data || count == 0)
        return {Result::ArgumentOutOfRange, "Invalid data pointer or count"};

    std::vector<uint8_t> bytes(sizeof(T) * count);
    std::memcpy(bytes.data(), data, bytes.size());

    return writeBytesToFile(path, bytes);
}

inline void Filesystem::registerProtocol(auto&& protocols)
{
    m_protocols = APH_FWD(protocols);
}

} // namespace aph
