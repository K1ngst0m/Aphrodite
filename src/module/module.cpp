#include "module.h"

#include "common/logger.h"

#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#else
#include <dlfcn.h>
#endif

namespace aph
{
Module::Module(Module&& other) noexcept
{
    *this = std::move(other);
}

auto Module::operator=(Module&& other) noexcept -> Module&
{
    close();
#ifdef _WIN32
    m_module       = other.m_module;
    other.m_module = nullptr;
#else
    m_dylib       = other.m_dylib;
    other.m_dylib = nullptr;
#endif
    return *this;
}

void Module::close()
{
#ifdef _WIN32
    if (m_module)
    {
        FreeLibrary(m_module);
    }
    m_module = nullptr;
#else
    if (m_dylib)
    {
        dlclose(m_dylib);
    }
    m_dylib = nullptr;
#endif
}

Module::~Module()
{
    close();
}

auto Module::getSymbolInternal(const char* symbol) -> void*
{
#ifdef _WIN32
    if (m_module)
    {
        return (void*)GetProcAddress(m_module, symbol);
    }
    else
    {
        return nullptr;
    }
#else
    if (m_dylib)
    {
        return dlsym(m_dylib, symbol);
    }
    return nullptr;
#endif
}

auto Module::open(std::string_view path) -> Result
{
#ifdef _WIN32
    m_module = LoadLibrary(path.data());
    if (!m_module)
    {
        return { Result::RuntimeError, "Failed to load dyndamic library." };
    }
#else
    m_dylib = dlopen(path.data(), RTLD_NOW);
    if (!m_dylib)
    {
        return { Result::RuntimeError, "Failed to load dyndamic library." };
    }
#endif
    return Result::Success;
}

auto Module::Create(std::string_view path) -> Expected<Module>
{
    Module module{};
    auto result = module.open(path);
    if (!result)
    {
        return {{}, result};
    }
    return { std::move(module) };
}
} // namespace aph
