#pragma once

#include "common/macros.h"
#include "common/smallVector.h"

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
        Info  = 1,
        Warn  = 2,
        Error = 3,
        None  = 4,
    };

    // Sink interface using type erasure instead of templates
    class ISink
    {
    public:
        virtual ~ISink()                           = default;
        virtual void write(const std::string& msg) = 0;
        virtual void flush()                       = 0;
    };

    // Template wrapper for creating sinks
    template <typename T>
    class SinkWrapper : public ISink
    {
    public:
        explicit SinkWrapper(T sink)
            : m_sink(std::move(sink))
        {
        }

        void write(const std::string& msg) override
        {
            m_sink.write(msg);
        }

        void flush() override
        {
            m_sink.flush();
        }

    private:
        T m_sink;
    };

    Logger();
    ~Logger();

    // Disable copy
    Logger(const Logger&)                    = delete;
    auto operator=(const Logger&) -> Logger& = delete;

    // Allow move
    Logger(Logger&&) noexcept;
    auto operator=(Logger&&) noexcept -> Logger&;

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
    [[nodiscard]] auto isInitialized() const -> bool;
    [[nodiscard]] auto getEnableLineInfo() const -> bool;

    // Sink management
    template <LogSinkConcept Sink>
    void addSink(Sink&& sink, bool isFileSink = false);

private:
    // Implementation detail helpers
    void logImpl(Level level, std::string_view message);

    // Private helper method for formatting log messages
    template <typename... Args>
    void logFormatted(Level level, std::string_view fmt, Args&&... args);

    // Helper methods for templates
    template <typename T>
    auto toFormat(const T& val) -> T;
    static auto toFormat(const char* val) -> const char*;
    static auto toFormat(const std::string& val) -> const char*;
    static auto toFormat(const std::filesystem::path& val) -> const char*;
    static auto toFormat(std::string_view val) -> const char*;

    // Function for sink management
    void addSinkWrapper(std::function<void(const std::string&)> writeFunc, std::function<void()> flushFunc,
                        bool isFileSink);

    // PIMPL implementation
    std::unique_ptr<LoggerImpl> m_impl;
};

template <typename T>
inline auto Logger::toFormat(const T& val) -> T
{
    return val;
}

// Required template implementations
template <typename... Args>
inline void Logger::debug(std::string_view fmt, Args&&... args)
{
    logFormatted(Level::Debug, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void Logger::info(std::string_view fmt, Args&&... args)
{
    logFormatted(Level::Info, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void Logger::warn(std::string_view fmt, Args&&... args)
{
    logFormatted(Level::Warn, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void Logger::error(std::string_view fmt, Args&&... args)
{
    logFormatted(Level::Error, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void Logger::logFormatted(Level level, std::string_view fmt, Args&&... args)
{
    constexpr size_t kBufferSize = 4096;
    aph::SmallVector<char, kBufferSize> buffer(kBufferSize);

    int result = std::snprintf(buffer.data(), kBufferSize, fmt.data(), toFormat(std::forward<Args>(args))...);

    std::string message;
    if (result >= 0)
    {
        if (static_cast<size_t>(result) < kBufferSize)
        {
            message = buffer.data();
        }
        else
        {
            // Resize and try again
            buffer.resize(result + 1);
            std::snprintf(buffer.data(), buffer.size(), fmt.data(), toFormat(std::forward<Args>(args))...);
            message = buffer.data();
        }
    }
    else
    {
        message = "Error formatting log message";
    }

    logImpl(level, message);
}

template <LogSinkConcept Sink>
inline void Logger::addSink(Sink&& sink, bool isFileSink)
{
    // Implementation using type erasure with lambda captures
    class SinkWrapper
    {
    public:
        explicit SinkWrapper(Sink s)
            : m_sink(std::move(s))
        {
        }

        void write(const std::string& msg)
        {
            m_sink.write(msg);
        }

        void flush()
        {
            m_sink.flush();
        }

    private:
        Sink m_sink;
    };

    // Create a shared_ptr to extend the lifetime of the sink
    auto sinkPtr = std::make_shared<SinkWrapper>(std::forward<Sink>(sink));

    // Add it via the implementation method
    addSinkWrapper(
        [sinkPtr](const std::string& msg)
        {
            sinkPtr->write(msg);
        },
        [sinkPtr]()
        {
            sinkPtr->flush();
        },
        isFileSink);
}

// Helper function for getting the active logger from GlobalManager
auto getActiveLogger() -> Logger*;

} // namespace aph

inline void LOG_FLUSH()
{
    if (auto* logger = ::aph::getActiveLogger())
    {
        logger->flush();
    }
}

#define GENERATE_LOG_FUNCS(TAG)                                                  \
    template <typename... Args>                                                  \
    APH_ALWAYS_INLINE void TAG##_LOG_DEBUG(std::string_view fmt, Args&&... args) \
    {                                                                            \
        if (auto* logger = ::aph::getActiveLogger())                             \
        {                                                                        \
            std::string combined = std::format("[{}] {}", #TAG, fmt);            \
            logger->debug(combined, std::forward<Args>(args)...);                \
        }                                                                        \
    }                                                                            \
    template <typename... Args>                                                  \
    APH_ALWAYS_INLINE void TAG##_LOG_WARN(std::string_view fmt, Args&&... args)  \
    {                                                                            \
        if (auto* logger = ::aph::getActiveLogger())                             \
        {                                                                        \
            std::string combined = std::format("[{}] {}", #TAG, fmt);            \
            logger->warn(combined, std::forward<Args>(args)...);                 \
        }                                                                        \
    }                                                                            \
    template <typename... Args>                                                  \
    APH_ALWAYS_INLINE void TAG##_LOG_INFO(std::string_view fmt, Args&&... args)  \
    {                                                                            \
        if (auto* logger = ::aph::getActiveLogger())                             \
        {                                                                        \
            std::string combined = std::format("[{}] {}", #TAG, fmt);            \
            logger->info(combined, std::forward<Args>(args)...);                 \
        }                                                                        \
    }                                                                            \
    template <typename... Args>                                                  \
    APH_ALWAYS_INLINE void TAG##_LOG_ERR(std::string_view fmt, Args&&... args)   \
    {                                                                            \
        if (auto* logger = ::aph::getActiveLogger())                             \
        {                                                                        \
            std::string combined = std::format("[{}] {}", #TAG, fmt);            \
            logger->error(combined, std::forward<Args>(args)...);                \
            logger->flush();                                                     \
        }                                                                        \
    }

GENERATE_LOG_FUNCS(CM)
GENERATE_LOG_FUNCS(VK)
GENERATE_LOG_FUNCS(MM)
GENERATE_LOG_FUNCS(APH)
