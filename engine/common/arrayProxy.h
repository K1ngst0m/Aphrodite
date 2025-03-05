#pragma once

namespace aph
{
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

    template <typename B = T, typename std::enable_if<std::is_const<B>::value, int>::type = 0>
    ArrayProxy(std::initializer_list<typename std::remove_const<T>::type> const& list) noexcept
        : m_count(static_cast<uint32_t>(list.size()))
        , m_ptr(list.begin())
    {
    }

#if __GNUC__ >= 9
#pragma GCC diagnostic pop
#endif

    // Any type with a .data() return type implicitly convertible to T*, and a .size() return type implicitly
    // convertible to size_t. The const version can capture temporaries, with lifetime ending at end of statement.
    template <typename V, typename std::enable_if<std::is_convertible<decltype(std::declval<V>().data()), T*>::value &&
                                                  std::is_convertible<decltype(std::declval<V>().size()),
                                                                      std::size_t>::value>::type* = nullptr>
    ArrayProxy(V const& v) noexcept
        : m_count(static_cast<uint32_t>(v.size()))
        , m_ptr(v.data())
    {
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

    template <
        typename B = T,
        typename std::enable_if<std::is_convertible<B, T>::value && std::is_lvalue_reference<B>::value, int>::type = 0>
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

    // Any l-value reference with a .data() return type implicitly convertible to T*, and a .size() return type implicitly convertible to size_t.
    template <typename V,
              typename std::enable_if<!std::is_convertible<decltype(std::declval<V>().begin()), T*>::value &&
                                          std::is_convertible<decltype(std::declval<V>().data()), T*>::value &&
                                          std::is_convertible<decltype(std::declval<V>().size()), std::size_t>::value &&
                                          std::is_lvalue_reference<V>::value,
                                      int>::type = 0>
    ArrayProxyNoTemporaries(V&& v) noexcept
        : m_count(static_cast<uint32_t>(v.size()))
        , m_ptr(v.data())
    {
    }

    // Any l-value reference with a .begin() return type implicitly convertible to T*, and a .size() return type implicitly convertible to size_t.
    template <typename V,
              typename std::enable_if<std::is_convertible<decltype(std::declval<V>().begin()), T*>::value &&
                                          std::is_convertible<decltype(std::declval<V>().size()), std::size_t>::value &&
                                          std::is_lvalue_reference<V>::value,
                                      int>::type = 0>
    ArrayProxyNoTemporaries(V&& v) noexcept
        : m_count(static_cast<uint32_t>(v.size()))
        , m_ptr(v.begin())
    {
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
