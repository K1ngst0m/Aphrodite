#ifndef APH_SMALL_VEC_H_
#define APH_SMALL_VEC_H_

#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

namespace aph
{

template <typename T, size_t MaxSize = 8, typename NonReboundT = T>
struct SmallBufferVectorAllocator
{
    alignas(alignof(T)) std::byte m_smallBuffer[MaxSize * sizeof(T)];
    std::allocator<T> m_alloc{};
    bool              m_smallBufferUsed = false;

    using value_type = T;
    // we have to set this three values, as they are responsible for the correct handling of the move assignment
    // operator
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_swap            = std::false_type;
    using is_always_equal                        = std::false_type;

    constexpr SmallBufferVectorAllocator() noexcept = default;
    template <class U>
    constexpr SmallBufferVectorAllocator(const SmallBufferVectorAllocator<U, MaxSize, NonReboundT>&) noexcept
    {
    }

    template <class U>
    struct rebind
    {
        typedef SmallBufferVectorAllocator<U, MaxSize, NonReboundT> other;
    };
    // don't copy the small buffer for the copy/move constructors, as the copying is done through the vector
    constexpr SmallBufferVectorAllocator(const SmallBufferVectorAllocator& other) noexcept :
        m_smallBufferUsed(other.m_smallBufferUsed)
    {
    }
    constexpr SmallBufferVectorAllocator& operator=(const SmallBufferVectorAllocator& other) noexcept
    {
        m_smallBufferUsed = other.m_smallBufferUsed;
        return *this;
    }
    constexpr SmallBufferVectorAllocator(SmallBufferVectorAllocator&&) noexcept {}
    constexpr SmallBufferVectorAllocator& operator=(const SmallBufferVectorAllocator&&) noexcept { return *this; }

    [[nodiscard]] constexpr T* allocate(const size_t n)
    {
        // when the allocator was rebound we don't want to use the small buffer
        if constexpr(std::is_same_v<T, NonReboundT>)
        {
            if(n <= MaxSize)
            {
                m_smallBufferUsed = true;
                // as long as we use less memory than the small buffer, we return a pointer to it
                return reinterpret_cast<T*>(&m_smallBuffer);
            }
        }
        m_smallBufferUsed = false;
        // otherwise use the default allocator
        return m_alloc.allocate(n);
    }
    constexpr void deallocate(void* p, const size_t n)
    {
        // we don't deallocate anything if the memory was allocated in small buffer
        if(&m_smallBuffer != p)
            m_alloc.deallocate(static_cast<T*>(p), n);
        m_smallBufferUsed = false;
    }

    // according to the C++ standard when propagate_on_container_move_assignment is set to false, the comparision
    // operators are used to check if two allocators are equal. When they are not, an element wise move is done instead
    // of just taking over the memory. For our implementation this means the comparision has to return false, when the
    // small buffer is active
    friend constexpr bool operator==(const SmallBufferVectorAllocator& lhs, const SmallBufferVectorAllocator& rhs)
    {
        return !lhs.m_smallBufferUsed && !rhs.m_smallBufferUsed;
    }
    friend constexpr bool operator!=(const SmallBufferVectorAllocator& lhs, const SmallBufferVectorAllocator& rhs)
    {
        return !(lhs == rhs);
    }
};

template <typename T, size_t N = 8>
class SmallVector : public std::vector<T, SmallBufferVectorAllocator<T, N>>
{
public:
    using vec = std::vector<T, SmallBufferVectorAllocator<T, N>>;
    // default initialize with the small buffer size
    constexpr SmallVector() noexcept { vec::reserve(N); }
    SmallVector(const SmallVector&)            = default;
    SmallVector& operator=(const SmallVector&) = default;
    SmallVector(SmallVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if(other.size() <= N)
            vec::reserve(N);
        vec::operator=(std::move(other));
    }
    SmallVector& operator=(SmallVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if(other.size() <= N)
            vec::reserve(N);
        vec::operator=(std::move(other));
        return *this;
    }
    // use the default constructor first to reserve then construct the values
    explicit SmallVector(size_t count) : SmallVector() { vec::resize(count); }
    SmallVector(size_t count, const T& value) : SmallVector() { vec::assign(count, value); }
    template <class InputIt>
    SmallVector(InputIt first, InputIt last) : SmallVector()
    {
        vec::insert(vec::begin(), first, last);
    }
    SmallVector(std::initializer_list<T> init) : SmallVector() { vec::insert(vec::begin(), init); }
    friend void swap(SmallVector& a, SmallVector& b) noexcept
    {
        using std::swap;
        swap(static_cast<vec&>(a), static_cast<vec&>(b));
    }
};
}  // namespace aph

#endif
