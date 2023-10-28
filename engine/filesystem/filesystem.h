#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <filesystem>
#include <string>
#include <map>
#include <memory>
#include <iostream>
#include <memory.h>
#include <sys/inotify.h>
#include <thread>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common/singleton.h"

namespace aph
{
class Filesystem final : public Singleton<Filesystem>
{
public:
    Filesystem();

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

    void registerProtocol(std::string_view protocol, const std::string& path);
    bool protocolExists(std::string_view protocol);
    void removeProtocol(std::string_view protocol);

    std::filesystem::path resolvePath(std::string_view inputPath);

private:
    std::map<int, std::function<void()>> m_callbacks;
    std::map<std::string, std::string>   m_protocols;
    std::map<void*, std::size_t>         m_mappedFiles;
    std::mutex                           m_mapLock;
};

}  // namespace aph

#endif  // FILESYSTEM_H_
