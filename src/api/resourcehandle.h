#pragma once

#include "common/timer.h"
namespace aph
{
namespace internal
{
inline size_t& GetTypeIdCounter() noexcept
{
    static size_t counter = 0;
    return counter;
}

template <typename T>
inline size_t GetTypeId() noexcept
{
    static size_t id = ++GetTypeIdCounter();
    return id;
}
// Type name utility without RTTI
#if APH_DEBUG
template <typename T>
constexpr const char* GetTypeName() noexcept
{
#if defined(__GNUC__) || defined(__clang__)
    std::string_view name = __PRETTY_FUNCTION__;
    auto pos = name.find("T = ") + 4;
    auto end = name.find_first_of(";]", pos);
    return name.substr(pos, end - pos).data();
#elif defined(_MSC_VER)
    std::string_view name = __FUNCSIG__;
    auto pos = name.find("GetTypeName<") + 12;
    auto end = name.find_first_of(">(", pos);
    return name.substr(pos, end - pos).data();
#else
    return "unknown_type";
#endif
}
#endif
} // namespace internal

struct DummyCreateInfo
{
    size_t typeId = 0;
};

struct DummyHandle
{
    size_t typeId = 0;
};

template <typename T_Handle = DummyHandle, typename T_CreateInfo = DummyCreateInfo>
class ResourceHandle
{
    enum LifeTime
    {
        eLifeTimeCreation,
    };

public:
    using HandleType = T_Handle;
    using CreateInfoType = T_CreateInfo;

    constexpr ResourceHandle() noexcept = default;

    constexpr ResourceHandle(HandleType handle, CreateInfoType createInfo = {}) noexcept
        : m_handle(handle)
        , m_createInfo(createInfo)
    {
        if constexpr (std::is_same_v<T_CreateInfo, DummyCreateInfo>)
        {
            m_createInfo.typeId = internal::GetTypeId<T_Handle>();
        }
        if constexpr (std::is_same_v<T_Handle, DummyHandle>)
        {
            m_handle.typeId = internal::GetTypeId<T_Handle>();
        }

#if APH_DEBUG
        m_timer.set(eLifeTimeCreation);
#endif
    }

    ResourceHandle(const ResourceHandle&) = delete;
    ResourceHandle& operator=(const ResourceHandle&) = delete;

    constexpr ResourceHandle(ResourceHandle&& other) noexcept
        : m_handle(std::exchange(other.m_handle, {}))
        , m_createInfo(std::move(other.m_createInfo))
#if APH_DEBUG
        , m_debugName(std::move(other.m_debugName))
        , m_timer(std::move(other.m_timer))
#endif
    {
    }

    constexpr ResourceHandle& operator=(ResourceHandle&& other) noexcept
    {
        if (this != &other)
        {
            m_handle = std::exchange(other.m_handle, {});
            m_createInfo = std::move(other.m_createInfo);
#if APH_DEBUG
            m_debugName = std::move(other.m_debugName);
            m_timer = std::move(other.m_timer);
#endif
        }
        return *this;
    }

    ~ResourceHandle() noexcept = default;

    constexpr operator T_Handle() const noexcept
    {
        return m_handle;
    }

    [[nodiscard]] constexpr T_Handle& getHandle() noexcept
    {
        return m_handle;
    }
    [[nodiscard]] constexpr const T_Handle& getHandle() const noexcept
    {
        return m_handle;
    }

    [[nodiscard]] constexpr T_CreateInfo& getCreateInfo() noexcept
    {
        return m_createInfo;
    }
    [[nodiscard]] constexpr const T_CreateInfo& getCreateInfo() const noexcept
    {
        return m_createInfo;
    }

#if APH_DEBUG
    void setDebugName(std::string_view name) noexcept
    {
        m_debugName = name;
    }

    [[nodiscard]] std::string_view getDebugName() const noexcept
    {
        return m_debugName;
    }

    void debugPrint(auto&& logFunc) const noexcept
    {
        auto age = m_timer.interval(eLifeTimeCreation);

        std::stringstream ss;
        ss << "ResourceHandle<" << internal::GetTypeName<T_Handle>()
           << ">: " << (m_debugName.empty() ? "[unnamed]" : m_debugName) << " | Age: " << age << "s"
           << " | Address: " << &m_handle;

        logFunc(ss.str());
    }

#else
    // Release-only minimal implementations
    constexpr void setDebugName(std::string_view) noexcept
    {
    }
    [[nodiscard]] constexpr std::string_view getDebugName() const noexcept
    {
        return {};
    }
#endif

protected:
    T_Handle m_handle = {};
    T_CreateInfo m_createInfo = {};

#if APH_DEBUG
    std::string m_debugName;
    Timer m_timer;
#endif
};

template <typename TObject>
concept ResourceHandleType = requires(TObject* obj, std::string_view name) {
    { obj->getDebugName() } -> std::convertible_to<std::string_view>;
    { obj->setDebugName(name) };
    { obj->getHandle() };
};
} // namespace aph
