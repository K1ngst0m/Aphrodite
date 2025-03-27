#pragma once

namespace aph
{
template <typename V, typename T>
concept ViewCompatible = requires(V v) {
    { v.data() } -> std::convertible_to<T*>;
    { v.size() } -> std::convertible_to<std::size_t>;
};

template <typename T>
class ArrayProxy
{
public:
    constexpr ArrayProxy() noexcept
        : m_count(0)
        , m_ptr(nullptr)
    {
    }

    constexpr ArrayProxy(std::nullptr_t) noexcept
        : m_count(0)
        , m_ptr(nullptr)
    {
    }

    ArrayProxy(T const& value) noexcept
        : m_count(1)
        , m_ptr(&value)
    {
    }

    ArrayProxy(uint32_t count, T const* ptr) noexcept
        : m_count(count)
        , m_ptr(ptr)
    {
    }

    template <std::size_t C>
    ArrayProxy(T const (&ptr)[C]) noexcept
        : m_count(C)
        , m_ptr(ptr)
    {
    }

#if __GNUC__ >= 9
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winit-list-lifetime"
#endif

    ArrayProxy(std::initializer_list<T> const& list) noexcept
        : m_count(static_cast<uint32_t>(list.size()))
        , m_ptr(list.begin())
    {
    }

    template <typename B = T>
        requires std::is_const_v<B>
    ArrayProxy(std::initializer_list<typename std::remove_const<T>::type> const& list) noexcept
        : m_count(static_cast<uint32_t>(list.size()))
        , m_ptr(list.begin())
    {
    }

#if __GNUC__ >= 9
#pragma GCC diagnostic pop
#endif

    // Any type with a .data() return type implicitly convertible to T*, and a .size() return type
    template <ViewCompatible<T> V>
    ArrayProxy(V const& v) noexcept
        : m_count(static_cast<uint32_t>(v.size()))
        , m_ptr(v.data())
    {
    }

    // Templated conversion operator to convert to any type constructible from iterators
    template <typename Container>
        requires std::is_constructible_v<Container, const T*, const T*> && 
                 (!std::is_same_v<Container, ArrayProxy<T>>)
    operator Container() const 
    {
        return Container(begin(), end());
    }

    const T* begin() const noexcept
    {
        return m_ptr;
    }

    const T* end() const noexcept
    {
        return m_ptr + m_count;
    }

    const T& front() const noexcept
    {
        assert(m_count && m_ptr);
        return *m_ptr;
    }

    const T& back() const noexcept
    {
        assert(m_count && m_ptr);
        return *(m_ptr + m_count - 1);
    }

    bool empty() const noexcept
    {
        return (m_count == 0);
    }

    uint32_t size() const noexcept
    {
        return m_count;
    }

    T const* data() const noexcept
    {
        return m_ptr;
    }

    const T& operator[](std::size_t idx) const
    {
        assert(idx < m_count);
        return m_ptr[idx];
    }

    bool operator==(const ArrayProxy<T>& rhs) const
    {
        if (m_count != rhs.m_count)
        {
            return false;
        }
        return std::equal(this->begin(), this->end(), rhs.begin());
    }

private:
    uint32_t m_count;
    T const* m_ptr;
};

template <typename T>
class ArrayProxyNoTemporaries
{
public:
    constexpr ArrayProxyNoTemporaries() noexcept
        : m_count(0)
        , m_ptr(nullptr)
    {
    }

    constexpr ArrayProxyNoTemporaries(std::nullptr_t) noexcept
        : m_count(0)
        , m_ptr(nullptr)
    {
    }

    template <typename B = T>
        requires std::is_convertible_v<B, T> && std::is_lvalue_reference_v<B>
    ArrayProxyNoTemporaries(B&& value) noexcept
        : m_count(1)
        , m_ptr(&value)
    {
    }

    ArrayProxyNoTemporaries(uint32_t count, T* ptr) noexcept
        : m_count(count)
        , m_ptr(ptr)
    {
    }

    template <std::size_t C>
    ArrayProxyNoTemporaries(T (&ptr)[C]) noexcept
        : m_count(C)
        , m_ptr(ptr)
    {
    }

    template <std::size_t C>
    ArrayProxyNoTemporaries(T (&&ptr)[C]) = delete;

    // For l-value references that meet the criteria
    template <ViewCompatible<T> V>
        requires std::is_lvalue_reference_v<V> && std::ranges::range<V>
    ArrayProxyNoTemporaries(V&& v) noexcept
        : m_count(static_cast<uint32_t>(v.size()))
        , m_ptr(v.begin() ? v.begin() : v.data())
    {
    }

    // Templated conversion operator to convert to any type constructible from iterators
    template <typename Container>
        requires std::is_constructible_v<Container, T*, T*> && 
                 (!std::is_same_v<Container, ArrayProxyNoTemporaries<T>>)
    operator Container() const 
    {
        return Container(begin(), end());
    }

    const T* begin() const noexcept
    {
        return m_ptr;
    }

    const T* end() const noexcept
    {
        return m_ptr + m_count;
    }

    const T& front() const noexcept
    {
        assert(m_count && m_ptr);
        return *m_ptr;
    }

    const T& back() const noexcept
    {
        assert(m_count && m_ptr);
        return *(m_ptr + m_count - 1);
    }

    bool empty() const noexcept
    {
        return (m_count == 0);
    }

    uint32_t size() const noexcept
    {
        return m_count;
    }

    T* data() const noexcept
    {
        return m_ptr;
    }

    const T& operator[](std::size_t idx) const
    {
        assert(idx < m_count);
        return m_ptr[idx];
    }

    bool operator==(const ArrayProxyNoTemporaries<T>& rhs) const
    {
        if (m_count != rhs.m_count)
        {
            return false;
        }
        return std::equal(this->begin(), this->end(), rhs.begin());
    }

private:
    uint32_t m_count;
    T* m_ptr;
};
} // namespace aph
