#pragma once

namespace aph
{
inline void CLICallbacks::setErrorHandler(auto&& func)
{
    m_errorHandler = APH_FWD(func);
}

inline void CLICallbacks::add(auto&& cli, auto&& func)
{
    m_callbacks[APH_FWD(cli)] = APH_FWD(func);
}

template <typename T>
inline Expected<T> CLIParser::next() const
{
    static_assert(dependent_false_v<T>, "Invalid value type of the argument.");
    return Expected<T>(Result::Code::RuntimeError, "Invalid type requested");
}

template <NumericType T>
inline Expected<T> CLIParser::next() const
{
    if (m_args.empty())
    {
        return Expected<T>(Result::Code::ArgumentOutOfRange, "Missing numeric argument");
    }

    T value{};
    const std::string& arg = m_args[0];

    // Safe conversion without exceptions
    if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>)
    {
        char* end = nullptr;
        errno = 0; // Reset errno before conversion
        const unsigned long long parsed = std::strtoull(arg.c_str(), &end, 10);

        if (end == arg.c_str() || *end != '\0' || errno == ERANGE ||
            parsed > static_cast<unsigned long long>(std::numeric_limits<T>::max()))
        {
            return Expected<T>(Result::Code::RuntimeError,
                            "Failed to parse unsigned integer value: out of range for requested type");
        }
        value = static_cast<T>(parsed);
    }
    else if constexpr (std::is_integral_v<T>)
    {
        char* end = nullptr;
        errno = 0; // Reset errno before conversion
        const long long parsed = std::strtoll(arg.c_str(), &end, 10);

        if (end == arg.c_str() || *end != '\0' || errno == ERANGE ||
            parsed > static_cast<long long>(std::numeric_limits<T>::max()) ||
            parsed < static_cast<long long>(std::numeric_limits<T>::min()))
        {
            return Expected<T>(Result::Code::RuntimeError,
                            "Failed to parse integer value: out of range for requested type");
        }
        value = static_cast<T>(parsed);
    }
    else // floating point
    {
        char* end = nullptr;
        errno = 0; // Reset errno before conversion
        const double parsed = std::strtod(arg.c_str(), &end);

        if (end == arg.c_str() || *end != '\0' || errno == ERANGE)
        {
            return Expected<T>(Result::Code::RuntimeError,
                            "Failed to parse floating-point value: out of range or invalid format");
        }

        // For floating point types, check if the parsed value fits in the target type
        if constexpr (!std::is_same_v<T, double>)
        {
            if (parsed > static_cast<double>(std::numeric_limits<T>::max()) ||
                parsed < static_cast<double>(std::numeric_limits<T>::lowest()))
            {
                return Expected<T>(Result::Code::RuntimeError, "Floating-point value out of range for target type");
            }
        }
        value = static_cast<T>(parsed);
    }

    m_args = m_args.subspan(1);
    return value;
}

template <StringType T>
inline Expected<T> CLIParser::next() const
{
    return nextString();
}

} // namespace aph
