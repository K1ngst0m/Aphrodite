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
Module::Module(const char* path)
{
    open(path);
}

Module::Module(Module&& other) noexcept
{
    *this = std::move(other);
}

Module& Module::operator=(Module&& other) noexcept
{
    close();
#ifdef _WIN32
    m_module = other.m_module;
    other.m_module = nullptr;
#else
    m_dylib = other.m_dylib;
    other.m_dylib = nullptr;
#endif
    return *this;
}

void Module::close()
{
#ifdef _WIN32
    if (m_module)
        FreeLibrary(m_module);
    m_module = nullptr;
#else
    if (m_dylib)
        dlclose(m_dylib);
    m_dylib = nullptr;
#endif
}

Module::~Module()
{
    close();
}

void* Module::getSymbolInternal(const char* symbol)
{
#ifdef _WIN32
    if (m_module)
        return (void*)GetProcAddress(m_module, symbol);
    else
        return nullptr;
#else
    if (m_dylib)
        return dlsym(m_dylib, symbol);
    return nullptr;
#endif
}
void Module::open(const char* path)
{
#ifdef _WIN32
    m_module = LoadLibrary(path);
    if (!m_module)
        CM_LOG_ERR("Failed to load dynamic library.\n");
#else
    m_dylib = dlopen(path, RTLD_NOW);
    if (!m_dylib)
        CM_LOG_ERR("Failed to load dynamic library.\n");
#endif
}
} // namespace aph
