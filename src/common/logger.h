#pragma once

#include "common/smallVector.h"
#include <vector>
#include <mutex>
#include <sstream>
#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <functional>

namespace aph
{

// Forward declaration for implementation details
class LoggerImpl;

template <typename T>
concept LogSinkConcept = requires(T t, const std::string& msg) {
    { t.write(msg) } -> std::same_as<void>;
    { t.flush() } -> std::same_as<void>;
};

class Logger
{
public:
    enum class Level : uint8_t
    {
        Debug = 0,
        Info = 1,
        Warn = 2,
        Error = 3,
        None = 4,
    };

    // Sink interface using type erasure instead of templates
    class ISink 
    {
    public:
        virtual ~ISink() = default;
        virtual void write(const std::string& msg) = 0;
        virtual void flush() = 0;
    };
    
    // Template wrapper for creating sinks
    template <typename T>
    class SinkWrapper : public ISink 
    {
    public:
        SinkWrapper(T sink) : m_sink(std::move(sink)) {}
        void write(const std::string& msg) override { m_sink.write(msg); }
        void flush() override { m_sink.flush(); }
    private:
        T m_sink;
    };

    Logger();
    ~Logger();
    
    // Disable copy
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Allow move
    Logger(Logger&&) noexcept;
    Logger& operator=(Logger&&) noexcept;

    // Core functionality
    void initialize();
    void flush();
    void setLogFile(const std::string& filename);
    
    // Log message methods - keep templates in header but use PIMPL for implementation
    template <typename... Args>
    void debug(std::string_view fmt, Args&&... args);
    
    template <typename... Args>
    void info(std::string_view fmt, Args&&... args);
    
    template <typename... Args>
    void warn(std::string_view fmt, Args&&... args);
    
    template <typename... Args>
    void error(std::string_view fmt, Args&&... args);
    
    // Configuration methods
    void setLogLevel(Level level);
    void setLogLevel(uint32_t level);
    void setEnableTime(bool value);
    void setEnableColor(bool value);
    void setEnableLineInfo(bool value);
    
    // State queries
    bool isInitialized() const;
    bool getEnableLineInfo() const;

    // Sink management
    template <LogSinkConcept Sink>
    void addSink(Sink&& sink, bool isFileSink = false);

private:
    // Implementation detail helpers
    void log_impl(Level level, std::string_view message);
    
    // Private helper method for formatting log messages
    template <typename... Args>
    void logFormatted(Level level, std::string_view fmt, Args&&... args);
    
    // Helper methods for templates
    template <typename T> T toFormat(const T& val) { return val; }
    const char* toFormat(const char* val) { return val; }
    const char* toFormat(const std::string& val) { return val.c_str(); }
    const char* toFormat(const std::filesystem::path& val) { return val.c_str(); }
    const char* toFormat(std::string_view val) { return val.data(); }
    
    // Function for sink management
    void addSinkWrapper(
        std::function<void(const std::string&)> writeFunc,
        std::function<void()> flushFunc,
        bool isFileSink
    );
    
    // PIMPL implementation
    std::unique_ptr<LoggerImpl> m_impl;
};

// Required template implementations
template <typename... Args>
inline void Logger::debug(std::string_view fmt, Args&&... args) {
    logFormatted(Level::Debug, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void Logger::info(std::string_view fmt, Args&&... args) {
    logFormatted(Level::Info, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void Logger::warn(std::string_view fmt, Args&&... args) {
    logFormatted(Level::Warn, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void Logger::error(std::string_view fmt, Args&&... args) {
    logFormatted(Level::Error, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void Logger::logFormatted(Level level, std::string_view fmt, Args&&... args) {
    constexpr size_t BUFFER_SIZE = 4096;
    aph::SmallVector<char, BUFFER_SIZE> buffer(BUFFER_SIZE);
    
    int result = std::snprintf(buffer.data(), BUFFER_SIZE, fmt.data(), toFormat(std::forward<Args>(args))...);
    
    std::string message;
    if (result >= 0) {
        if (static_cast<size_t>(result) < BUFFER_SIZE) {
            message = buffer.data();
        } else {
            // Resize and try again
            buffer.resize(result + 1);
            std::snprintf(buffer.data(), buffer.size(), fmt.data(), toFormat(std::forward<Args>(args))...);
            message = buffer.data();
        }
    } else {
        message = "Error formatting log message";
    }
    
    log_impl(level, message);
}

template <LogSinkConcept Sink>
inline void Logger::addSink(Sink&& sink, bool isFileSink) {
    // Implementation using type erasure with lambda captures
    class SinkWrapper {
    public:
        SinkWrapper(Sink s) : sink(std::move(s)) {}
        void write(const std::string& msg) { sink.write(msg); }
        void flush() { sink.flush(); }
    private:
        Sink sink;
    };
    
    // Create a shared_ptr to extend the lifetime of the sink
    auto sinkPtr = std::make_shared<SinkWrapper>(std::forward<Sink>(sink));
    
    // Add it via the implementation method
    addSinkWrapper(
        [sinkPtr](const std::string& msg) { sinkPtr->write(msg); },
        [sinkPtr]() { sinkPtr->flush(); },
        isFileSink
    );
}

// Helper function for getting the active logger from GlobalManager
Logger* getActiveLogger();

} // namespace aph

inline void LOG_FLUSH()
{
    if (auto* logger = ::aph::getActiveLogger())
    {
        logger->flush();
    }
}

#define GENERATE_LOG_FUNCS(TAG)                                                   \
    template <typename... Args>                                                   \
    void TAG##_LOG_DEBUG(std::string_view fmt, Args&&... args)                    \
    {                                                                             \
        if (auto* logger = ::aph::getActiveLogger()) {                            \
            std::string combined = std::format("[{}] {}", #TAG, fmt);             \
            logger->debug(combined, std::forward<Args>(args)...);                 \
        }                                                                         \
    }                                                                             \
    template <typename... Args>                                                   \
    void TAG##_LOG_WARN(std::string_view fmt, Args&&... args)                     \
    {                                                                             \
        if (auto* logger = ::aph::getActiveLogger()) {                            \
            std::string combined = std::format("[{}] {}", #TAG, fmt);             \
            logger->warn(combined, std::forward<Args>(args)...);                  \
        }                                                                         \
    }                                                                             \
    template <typename... Args>                                                   \
    void TAG##_LOG_INFO(std::string_view fmt, Args&&... args)                     \
    {                                                                             \
        if (auto* logger = ::aph::getActiveLogger()) {                            \
            std::string combined = std::format("[{}] {}", #TAG, fmt);             \
            logger->info(combined, std::forward<Args>(args)...);                  \
        }                                                                         \
    }                                                                             \
    template <typename... Args>                                                   \
    void TAG##_LOG_ERR(std::string_view fmt, Args&&... args)                      \
    {                                                                             \
        if (auto* logger = ::aph::getActiveLogger()) {                            \
            std::string combined = std::format("[{}] {}", #TAG, fmt);             \
            logger->error(combined, std::forward<Args>(args)...);                 \
            logger->flush();                                                      \
        }                                                                         \
    }

GENERATE_LOG_FUNCS(CM)
GENERATE_LOG_FUNCS(VK)
GENERATE_LOG_FUNCS(MM)
