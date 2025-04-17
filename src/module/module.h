#pragma once

#include "common/result.h"

namespace aph
{
class Module
{
public:
    Module() = default;
    ~Module();

    Module(Module&& other) noexcept;
    auto operator=(Module&& other) noexcept -> Module&;

    template <typename Func>
    auto getSymbol(const char* symbol) -> Func;

    auto open(std::string_view path) -> Result;
    void close();

    explicit operator bool() const
    {
#if _WIN32
        return m_module != nullptr;
#else
        return m_dylib != nullptr;
#endif
    }

private:
#if _WIN32
    HMODULE m_module = nullptr;
#else
    void* m_dylib = nullptr;
#endif

    auto getSymbolInternal(const char* symbol) -> void*;

    static auto Create(std::string_view path) -> Expected<Module>;
};

template <typename Func>
inline auto Module::getSymbol(const char* symbol) -> Func
{
    return reinterpret_cast<Func>(getSymbolInternal(symbol));
}

} // namespace aph
