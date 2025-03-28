#include "filesystem.h"
#include "common/logger.h"

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

bool Filesystem::protocolExists(const std::string& protocol)
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

std::filesystem::path Filesystem::resolvePath(std::string_view inputPath) const
{
    if (auto protocolEnd = inputPath.find("://"); protocolEnd != std::string::npos)
    {
        // TODO
        std::string protocol = std::string{ inputPath.substr(0, protocolEnd) };
        std::string relativePath = std::string{ inputPath.substr(protocolEnd + 3) };
        if (!m_protocols.contains(protocol))
        {
            CM_LOG_ERR("Unknown protocol: %s", protocol);
            return {};
        }
        return getCurrentWorkingDirectory() / std::filesystem::path(m_protocols.at(protocol)) / relativePath;
    }

    return inputPath;
}

void* Filesystem::map(std::string_view path)
{
    std::lock_guard<std::mutex> lock(m_mapLock);
    auto resolvedPath = resolvePath(path);

    int fd = open(resolvedPath.string().c_str(), O_RDONLY);
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

std::string Filesystem::readFileToString(std::string_view path)
{
    std::ifstream file(resolvePath(path), std::ios::in);
    if (!file)
    {
        CM_LOG_ERR("Unable to open file: %s", path);
        return {};
    }
    return { (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>() };
}
std::vector<uint8_t> Filesystem::readFileToBytes(std::string_view path)
{
    std::ifstream file(resolvePath(path), std::ios::binary | std::ios::ate);
    if (!file)
    {
        CM_LOG_ERR("Unable to open file: %s", path);
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read((char*)buffer.data(), size))
    {
        CM_LOG_ERR("Error reading file: %s", path);
        return {};
    }
    return buffer;
}
std::vector<std::string> Filesystem::readFileLines(std::string_view path)
{
    std::ifstream file(resolvePath(path));
    if (!file)
    {
        CM_LOG_ERR("Unable to open file: %s", path);
        return {};
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))
    {
        lines.push_back(line);
    }
    return lines;
}
void Filesystem::writeStringToFile(std::string_view path, const std::string& content)
{
    std::ofstream file(resolvePath(path).string(), std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Failed to open file for writing: " + std::string(path));
    }
    file << content;
}
void Filesystem::writeBytesToFile(std::string_view path, const std::vector<uint8_t>& bytes)
{
    std::ofstream file(resolvePath(path).string(), std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Failed to open file for writing: " + std::string(path));
    }
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}
void Filesystem::writeLinesToFile(std::string_view path, const std::vector<std::string>& lines)
{
    std::ofstream file(resolvePath(path).string());
    if (!file)
    {
        throw std::runtime_error("Failed to open file for writing: " + std::string(path));
    }
    for (const auto& line : lines)
    {
        file << line << '\n';
    }
}
std::filesystem::path Filesystem::getCurrentWorkingDirectory() const
{
    return std::filesystem::current_path();
}

bool Filesystem::exist(std::string_view path) const
{
    auto resolvedPath = resolvePath(path);
    return std::filesystem::exists(resolvedPath);
}

bool Filesystem::createDirectories(std::string_view path) const
{
    std::error_code ec;
    auto resolvedPath = resolvePath(path);
    auto result = std::filesystem::create_directories(resolvedPath, ec);
    if (ec)
    {
        CM_LOG_ERR("Failed to create directories: %s, error: %s", path, ec.message().c_str());
        return false;
    }
    return result;
}

int64_t Filesystem::getLastModifiedTime(std::string_view path) const
{
    auto resolvedPath = resolvePath(path);

    std::error_code ec;
    auto fileTime = std::filesystem::last_write_time(resolvedPath, ec);

    if (ec)
    {
        CM_LOG_WARN("Failed to get last modified time for %s: %s", path.data(), ec.message().c_str());
        return 0;
    }

    return fileTime.time_since_epoch().count();
}
} // namespace aph
