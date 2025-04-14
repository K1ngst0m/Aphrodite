#pragma once

#include "common/timer.h"

namespace aph
{

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
    using HandleType     = T_Handle;
    using CreateInfoType = T_CreateInfo;

    constexpr ResourceHandle() noexcept = default;

    constexpr explicit ResourceHandle(HandleType handle, CreateInfoType createInfo = {}) noexcept;

    ResourceHandle(const ResourceHandle&)                    = delete;
    auto operator=(const ResourceHandle&) -> ResourceHandle& = delete;

    constexpr ResourceHandle(ResourceHandle&& other) noexcept;

    constexpr auto operator=(ResourceHandle&& other) noexcept -> ResourceHandle&;

    ~ResourceHandle() noexcept = default;

    constexpr explicit operator T_Handle() const noexcept;

    [[nodiscard]] constexpr auto getHandle() noexcept -> T_Handle&;
    [[nodiscard]] constexpr auto getHandle() const noexcept -> const T_Handle&;
    [[nodiscard]] constexpr auto getCreateInfo() noexcept -> T_CreateInfo&;
    [[nodiscard]] constexpr auto getCreateInfo() const noexcept -> const T_CreateInfo&;

#if APH_DEBUG
    auto setDebugName(std::string_view name) noexcept -> void;

    [[nodiscard]] auto getDebugName() const noexcept -> std::string_view;

    auto debugPrint(auto&& logFunc) const noexcept -> void;

#else
    // Release-only minimal implementations
    constexpr auto setDebugName(std::string_view) noexcept -> void
    {
    }
    [[nodiscard]] constexpr auto getDebugName() const noexcept -> std::string_view
    {
        return {};
    }
    constexpr auto debugPrint(auto&&) const noexcept -> void
    {
    }
#endif

protected:
    T_Handle m_handle         = {};
    T_CreateInfo m_createInfo = {};

#if APH_DEBUG
    std::string m_debugName;
    Timer m_timer;
#endif
};

namespace internal
{
inline auto GetTypeIdCounter() noexcept -> size_t&
{
    static size_t counter = 0;
    return counter;
}

template <typename T>
inline auto GetTypeId() noexcept -> size_t
{
    static size_t id = ++GetTypeIdCounter();
    return id;
}
// Type name utility without RTTI
#if APH_DEBUG
template <typename T>
constexpr auto GetTypeName() noexcept -> const char*
{
#if defined(__GNUC__) || defined(__clang__)
    std::string_view name = __PRETTY_FUNCTION__;
    auto pos              = name.find("T = ") + 4;
    auto end              = name.find_first_of(";]", pos);
    return name.substr(pos, end - pos).data();
#elif defined(_MSC_VER)
    std::string_view name = __FUNCSIG__;
    auto pos              = name.find("GetTypeName<") + 12;
    auto end              = name.find_first_of(">(", pos);
    return name.substr(pos, end - pos).data();
#else
    return "unknown_type";
#endif
}
#endif
} // namespace internal

#if APH_DEBUG
template <typename THandle, typename TCreateInfo>
inline auto ResourceHandle<THandle, TCreateInfo>::debugPrint(auto&& logFunc) const noexcept -> void
{
    auto age = m_timer.interval(eLifeTimeCreation);

    std::stringstream ss;
    ss << "ResourceHandle<" << internal::GetTypeName<THandle>()
       << ">: " << (m_debugName.empty() ? "[unnamed]" : m_debugName) << " | Age: " << age << "s"
       << " | Address: " << &m_handle;

    logFunc(ss.str());
}

template <typename THandle, typename TCreateInfo>
inline auto ResourceHandle<THandle, TCreateInfo>::getDebugName() const noexcept -> std::string_view
{
    return m_debugName;
}

template <typename THandle, typename TCreateInfo>
inline auto ResourceHandle<THandle, TCreateInfo>::setDebugName(std::string_view name) noexcept -> void
{
    m_debugName = name;
}
#endif

template <typename THandle, typename TCreateInfo>
constexpr auto ResourceHandle<THandle, TCreateInfo>::getCreateInfo() const noexcept -> const TCreateInfo&
{
    return m_createInfo;
}

template <typename THandle, typename TCreateInfo>
constexpr auto ResourceHandle<THandle, TCreateInfo>::getCreateInfo() noexcept -> TCreateInfo&
{
    return m_createInfo;
}

template <typename THandle, typename TCreateInfo>
constexpr auto ResourceHandle<THandle, TCreateInfo>::getHandle() const noexcept -> const THandle&
{
    return m_handle;
}

template <typename THandle, typename TCreateInfo>
constexpr auto ResourceHandle<THandle, TCreateInfo>::getHandle() noexcept -> THandle&
{
    return m_handle;
}

template <typename THandle, typename TCreateInfo>
constexpr ResourceHandle<THandle, TCreateInfo>::operator THandle() const noexcept
{
    return m_handle;
}

template <typename THandle, typename TCreateInfo>
constexpr auto ResourceHandle<THandle, TCreateInfo>::operator=(ResourceHandle&& other) noexcept -> ResourceHandle&
{
    if (this != &other)
    {
        m_handle     = std::exchange(other.m_handle, {});
        m_createInfo = std::move(other.m_createInfo);
#if APH_DEBUG
        m_debugName = std::move(other.m_debugName);
        m_timer     = std::move(other.m_timer);
#endif
    }
    return *this;
}

template <typename THandle, typename TCreateInfo>
constexpr ResourceHandle<THandle, TCreateInfo>::ResourceHandle(ResourceHandle&& other) noexcept
    : m_handle(std::exchange(other.m_handle, {}))
    , m_createInfo(std::move(other.m_createInfo))
#if APH_DEBUG
    , m_debugName(std::move(other.m_debugName))
    , m_timer(std::move(other.m_timer))
#endif
{
}

template <typename THandle, typename TCreateInfo>
constexpr ResourceHandle<THandle, TCreateInfo>::ResourceHandle(HandleType handle, CreateInfoType createInfo) noexcept
    : m_handle(handle)
    , m_createInfo(createInfo)
{
    if constexpr (std::is_same_v<TCreateInfo, DummyCreateInfo>)
    {
        m_createInfo.typeId = internal::GetTypeId<THandle>();
    }
    if constexpr (std::is_same_v<THandle, DummyHandle>)
    {
        m_handle.typeId = internal::GetTypeId<THandle>();
    }

#if APH_DEBUG
    m_timer.set(eLifeTimeCreation);
#endif
}

template <typename TObject>
concept ResourceHandleType = requires(TObject* obj, std::string_view name) {
    { obj->getDebugName() } -> std::convertible_to<std::string_view>;
    { obj->setDebugName(name) };
    { obj->getHandle() };
};
} // namespace aph
