#pragma once
#include <coroutine>

namespace aph
{
template <typename T>
struct Generator
{
    struct promise_type
    {
        T current_value;

        Generator get_return_object()
        {
            return Generator{ Handle::from_promise(*this) };
        }
        std::suspend_always initial_suspend() noexcept
        {
            return {};
        }
        std::suspend_always final_suspend() noexcept
        {
            return {};
        }
        std::suspend_always yield_value(T value) noexcept
        {
            current_value = value;
            return {};
        }
        void return_void() noexcept
        {
        }
        void unhandled_exception()
        {
            std::terminate();
        }
    };

    using Handle = std::coroutine_handle<promise_type>;

    explicit Generator(Handle handle)
        : handle(handle)
    {
    }
    ~Generator()
    {
        if (handle)
            handle.destroy();
    }

    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;

    Generator(Generator&& other) noexcept
        : handle(other.handle)
    {
        other.handle = nullptr;
    }
    Generator& operator=(Generator&& other) noexcept
    {
        if (this != &other)
        {
            if (handle)
                handle.destroy();
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }

    struct iterator
    {
        Handle handle;

        iterator(Handle handle)
            : handle(handle)
        {
        }
        iterator& operator++()
        {
            handle.resume();
            return *this;
        }
        T operator*() const
        {
            return handle.promise().current_value;
        }
        bool operator==(std::default_sentinel_t) const
        {
            return !handle || handle.done();
        }
    };

    iterator begin()
    {
        if (handle)
            handle.resume();
        return iterator{ handle };
    }

    std::default_sentinel_t end()
    {
        return {};
    }

private:
    Handle handle;
};
} // namespace aph
