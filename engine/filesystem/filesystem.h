#pragma once

#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <memory.h>
#include <string>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <type_traits>
#include <unistd.h>
#include <utility>

#include "common/hash.h"
#include "common/logger.h"
#include "common/singleton.h"

namespace aph
{
class Filesystem final : public Singleton<Filesystem>
{
public:
    ~Filesystem() final;

    void* map(std::string_view path);
    void unmap(void* data);
    void clearMappedFiles();

    bool exist(std::string_view path) const;
    bool createDirectories(std::string_view path) const;

    std::string readFileToString(std::string_view path);
    std::vector<uint8_t> readFileToBytes(std::string_view path);
    std::vector<std::string> readFileLines(std::string_view path);

    void writeStringToFile(std::string_view path, const std::string& content);
    void writeBytesToFile(std::string_view path, const std::vector<uint8_t>& bytes);
    void writeLinesToFile(std::string_view path, const std::vector<std::string>& lines);

    template <typename T>
    bool writeBinaryData(std::string_view path, const T* data, size_t count) const
    {
        if (!data || count == 0)
            return false;

        std::vector<uint8_t> bytes(sizeof(T) * count);
        std::memcpy(bytes.data(), data, bytes.size());

        std::ofstream file(resolvePath(path).string(), std::ios::binary);
        if (!file)
        {
            CM_LOG_ERR("Failed to open file for writing: %s", path);
            return false;
        }

        file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        return file.good();
    }

    template <typename T>
    bool readBinaryData(std::string_view path, T* data, size_t count) const
    {
        if (!exist(path) || !data || count == 0)
            return false;

        auto bytes = const_cast<Filesystem*>(this)->readFileToBytes(path);
        if (bytes.size() < sizeof(T) * count)
            return false;

        std::memcpy(data, bytes.data(), sizeof(T) * count);
        return true;
    }

    void registerProtocol(auto&& protocols)
    {
        m_protocols = APH_FWD(protocols);
    }
    void registerProtocol(const std::string& protocol, const std::string& path);
    bool protocolExists(const std::string& protocol);
    void removeProtocol(const std::string& protocol);

    std::filesystem::path resolvePath(std::string_view inputPath) const;
    std::filesystem::path getCurrentWorkingDirectory() const;

    // Get the last modification time of a file
    int64_t getLastModifiedTime(std::string_view path) const;

private:
    HashMap<int, std::function<void()>> m_callbacks;
    HashMap<std::string, std::string> m_protocols;
    HashMap<void*, std::size_t> m_mappedFiles;
    std::mutex m_mapLock;
};

} // namespace aph
