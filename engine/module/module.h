#pragma once

namespace aph
{
class Module
{
public:
    Module() = default;
    explicit Module(const char* path);
    ~Module();

    Module(Module&& other) noexcept;
    Module& operator=(Module&& other) noexcept;

    template <typename Func>
    Func getSymbol(const char* symbol)
    {
        return reinterpret_cast<Func>(getSymbolInternal(symbol));
    }

    void open(const char* path);
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

    void* getSymbolInternal(const char* symbol);
};
} // namespace aph
