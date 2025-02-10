#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <filesystem>
#include <string>
#include <type_traits>
#include <utility>
#include <memory.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common/hash.h"
#include "common/singleton.h"

namespace aph
{
class Filesystem final : public Singleton<Filesystem>
{
public:
    ~Filesystem() final;

    void* map(std::string_view path);
    void  unmap(void* data);
    void  clearMappedFiles();

    std::string              readFileToString(std::string_view path);
    std::vector<uint8_t>     readFileToBytes(std::string_view path);
    std::vector<std::string> readFileLines(std::string_view path);

    void writeStringToFile(std::string_view path, const std::string& content);
    void writeBytesToFile(std::string_view path, const std::vector<uint8_t>& bytes);
    void writeLinesToFile(std::string_view path, const std::vector<std::string>& lines);

    template <typename T>
    requires std::is_same_v<std::remove_cvref_t<T>, HashMap<std::string, std::string>>
    void registerProtocol(T&& protocols)
    {
        m_protocols = std::forward<T>(protocols);
    }
    void registerProtocol(const std::string& protocol, const std::string& path);
    bool protocolExists(const std::string& protocol);
    void removeProtocol(const std::string& protocol);

    std::filesystem::path resolvePath(std::string_view inputPath);
    std::filesystem::path getCurrentWorkingDirectory();

private:
    HashMap<int, std::function<void()>> m_callbacks;
    HashMap<std::string, std::string>   m_protocols;
    HashMap<void*, std::size_t>         m_mappedFiles;
    std::mutex                           m_mapLock;
};

}  // namespace aph

#endif  // FILESYSTEM_H_
