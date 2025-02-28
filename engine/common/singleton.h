#ifndef APH_SINGLETON_H
#define APH_SINGLETON_H

#include <memory>
#include <mutex>
#include <stdexcept>

namespace aph
{

template <typename Derived>
class Singleton
{
public:
    static Derived& GetInstance()
    {
        // Meyers' Singleton with guaranteed thread-safety since C++11
        static Derived instance;
        return instance;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) noexcept = delete;
    Singleton& operator=(Singleton&&) noexcept = delete;

protected:
    Singleton() = default;
    virtual ~Singleton() = default;
};

} // namespace aph
#endif
