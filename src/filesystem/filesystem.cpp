#include "filesystem.h"

#include "common/logger.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace aph
{
Filesystem::~Filesystem()
{
    clearMappedFiles();
}

void Filesystem::registerProtocol(const std::string& protocol, const std::string& path)
{
    if (protocolExists(protocol))
    {
        CM_LOG_WARN("overrided the existing protocol %s. path: %s -> %s", protocol, m_protocols[protocol], path);
    }
    m_protocols[protocol] = path;
}

auto Filesystem::protocolExists(const std::string& protocol) const -> bool
{
    return m_protocols.contains(protocol);
}

void Filesystem::removeProtocol(const std::string& protocol)
{
    m_protocols.erase(protocol);
}

void Filesystem::clearMappedFiles()
{
    for (auto& [data, size] : m_mappedFiles)
    {
        munmap(data, size);
    }
    m_mappedFiles.clear();
}

auto Filesystem::resolvePath(std::string_view inputPath) const -> Expected<std::string>
{
    if (auto protocolEnd = inputPath.find("://"); protocolEnd != std::string::npos)
    {
        std::string protocol     = std::string{inputPath.substr(0, protocolEnd)};
        std::string relativePath = std::string{inputPath.substr(protocolEnd + 3)};
        if (!m_protocols.contains(protocol))
        {
            return {Result::RuntimeError, "Unknown protocol: " + protocol};
        }
        return getCurrentWorkingDirectory() + "/" + m_protocols.at(protocol) + "/" + relativePath;
    }

    return std::string{inputPath};
}

auto Filesystem::map(std::string_view path) -> void*
{
    std::lock_guard<std::mutex> lock(m_mapLock);
    auto resolvedPath = resolvePath(path);

    int fd = open(resolvedPath.value().c_str(), O_RDONLY);
    if (fd == -1)
        return nullptr;

    struct stat fileStat;
    if (fstat(fd, &fileStat) == -1)
    {
        close(fd);
        return nullptr;
    }

    void* mappedData = mmap(0, fileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (mappedData == MAP_FAILED)
    {
        return nullptr;
    }

    m_mappedFiles[mappedData] = fileStat.st_size;
    return mappedData;
}

void Filesystem::unmap(void* data)
{
    if (m_mappedFiles.find(data) != m_mappedFiles.end())
    {
        munmap(data, m_mappedFiles[data]);
        m_mappedFiles.erase(data);
    }
}

auto Filesystem::readFileToString(std::string_view path) const -> Expected<std::string>
{
    std::ifstream file(resolvePath(path).value(), std::ios::in);
    if (!file)
    {
        return {Result::RuntimeError, std::format("Unable to open file: {}", path)};
    }

    return std::string{(std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()};
}

auto Filesystem::readFileToBytes(std::string_view path) const -> Expected<std::vector<uint8_t>>
{
    auto resolvedPath = resolvePath(path);
    if (!resolvedPath.success())
    {
        return Expected<std::vector<uint8_t>>(Result::RuntimeError, std::string(resolvedPath.error().toString()));
    }

    std::vector<uint8_t> out;
    std::ifstream file(resolvedPath.value(), std::ios::binary);
    if (!file.is_open())
    {
        return Expected<std::vector<uint8_t>>(Result::RuntimeError, "Cannot open file: " + std::string(path));
    }

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    if (size == -1)
    {
        return Expected<std::vector<uint8_t>>(Result::RuntimeError,
                                              "Failed to get file size for: " + std::string(path));
    }
    file.seekg(0, std::ios::beg);

    out.resize(size);
    if (size > 0)
    {
        file.read(reinterpret_cast<char*>(out.data()), size);
        if (!file)
        {
            return Expected<std::vector<uint8_t>>(Result::RuntimeError,
                                                  "Failed to read complete file: " + std::string(path));
        }
    }

    return out;
}

auto Filesystem::readFileLines(std::string_view path) const -> Expected<std::vector<std::string>>
{
    auto resolvedPath = resolvePath(path);
    if (!resolvedPath.success())
    {
        return Expected<std::vector<std::string>>(Result::RuntimeError, std::string(resolvedPath.error().toString()));
    }

    std::ifstream file(resolvedPath.value());
    if (!file.is_open())
    {
        return Expected<std::vector<std::string>>(Result::RuntimeError, "Cannot open file: " + std::string(path));
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))
    {
        lines.push_back(line);
    }
    return lines;
}

auto Filesystem::writeStringToFile(std::string_view path, const std::string& content) const -> Result
{
    auto resolvedPath = resolvePath(path);
    if (!resolvedPath.success())
    {
        return {Result::RuntimeError, std::string(resolvedPath.error().toString())};
    }

    std::ofstream file(resolvedPath.value(), std::ios::binary);
    if (!file)
    {
        return {Result::RuntimeError, "Failed to open file for writing: " + std::string(path)};
    }
    file << content;

    if (!file.good())
    {
        return {Result::RuntimeError, "Failed to write to file: " + std::string(path)};
    }

    return Result::Success;
}

auto Filesystem::writeBytesToFile(std::string_view path, const std::vector<uint8_t>& bytes) const -> Result
{
    auto resolvedPath = resolvePath(path);
    if (!resolvedPath.success())
    {
        return {Result::RuntimeError, std::string(resolvedPath.error().toString())};
    }

    std::ofstream file(resolvedPath.value(), std::ios::binary);
    if (!file)
    {
        return {Result::RuntimeError, "Failed to open file for writing: " + std::string(path)};
    }

    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());

    if (!file.good())
    {
        return {Result::RuntimeError, "Failed to write to file: " + std::string(path)};
    }

    return Result::Success;
}

auto Filesystem::writeLinesToFile(std::string_view path, const std::vector<std::string>& lines) const -> Result
{
    auto resolvedPath = resolvePath(path);
    if (!resolvedPath.success())
    {
        return {Result::RuntimeError, std::string(resolvedPath.error().toString())};
    }

    std::ofstream file(resolvedPath.value());
    if (!file)
    {
        return {Result::RuntimeError, "Failed to open file for writing: " + std::string(path)};
    }

    for (const auto& line : lines)
    {
        file << line << '\n';
    }

    if (!file.good())
    {
        return {Result::RuntimeError, "Failed to write lines to file: " + std::string(path)};
    }

    return Result::Success;
}

auto Filesystem::getCurrentWorkingDirectory() const -> std::string
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != nullptr)
    {
        return std::string(cwd);
    }
    return ".";
}

auto Filesystem::exist(std::string_view path) const -> bool
{
    auto resolvedPath = resolvePath(path);
    struct stat buffer;
    return (stat(resolvedPath.value().c_str(), &buffer) == 0);
}

auto Filesystem::createDirectories(std::string_view path) const -> Result
{
    auto resolvedPath = resolvePath(path);

    // Split the path into components
    std::vector<std::string> components;
    std::string current;

    for (char c : resolvedPath.value())
    {
        if (c == '/')
        {
            if (!current.empty())
            {
                components.push_back(current);
                current.clear();
            }
        }
        else
        {
            current += c;
        }
    }

    if (!current.empty())
    {
        components.push_back(current);
    }

    // Create directories incrementally
    std::string buildPath = resolvedPath.value().front() == '/' ? "/" : "";

    for (size_t i = 0; i < components.size(); ++i)
    {
        buildPath += components[i];

        // Skip if this is the last component and not intended to be a directory
        if (i == components.size() - 1 && resolvedPath.value().back() != '/')
        {
            break;
        }

        // Create directory if it doesn't exist
        struct stat buffer;
        if (stat(buildPath.c_str(), &buffer) != 0)
        {
            if (mkdir(buildPath.c_str(), 0755) != 0)
            {
                std::string errorMsg = "Failed to create directory: " + buildPath + " - error " +
                                       std::to_string(errno) + ": " + strerror(errno);
                CM_LOG_ERR("%s", errorMsg.c_str());
                return {Result::RuntimeError, errorMsg};
            }
        }

        buildPath += "/";
    }

    return Result::Success;
}

auto Filesystem::getLastModifiedTime(std::string_view path) const -> int64_t
{
    auto resolvedPath = resolvePath(path);

    struct stat buffer;
    if (stat(resolvedPath.value().c_str(), &buffer) != 0)
    {
        CM_LOG_WARN("Failed to get last modified time for %s", path.data());
        return 0;
    }

    return static_cast<int64_t>(buffer.st_mtime);
}

auto Filesystem::getFileSize(std::string_view path) const -> size_t
{
    auto resolvedPath = resolvePath(path);

    struct stat buffer;
    if (stat(resolvedPath.value().c_str(), &buffer) != 0)
    {
        CM_LOG_WARN("Failed to get file size for %s", path.data());
        return 0;
    }

    return static_cast<size_t>(buffer.st_size);
}

auto Filesystem::getFileExtension(std::string_view path) const -> std::string
{
    auto lastDot = path.find_last_of('.');
    if (lastDot != std::string::npos)
    {
        return std::string(path.substr(lastDot));
    }
    return "";
}

auto Filesystem::absolutePath(std::string_view inputPath) const -> std::string
{
    auto resolved = resolvePath(inputPath);
    if (resolved.value().front() == '/')
    {
        return resolved.value();
    }
    return getCurrentWorkingDirectory() + "/" + resolved.value();
}
} // namespace aph
