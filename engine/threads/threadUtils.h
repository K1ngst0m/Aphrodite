#ifndef APH_THREAD_UTILS_H
#define APH_THREAD_UTILS_H
#include <pthread.h>

namespace aph::thread
{
template <typename T>
    requires std::is_constructible_v<std::string, T>
inline void setName(T&& name)
{
    std::string s(std::forward<T>(name));
    [[maybe_unused]] int result = pthread_setname_np(pthread_self(), s.c_str());
    assert(result == 0);
}

inline std::string getName()
{
    std::string name;
    name.resize(256);
    pthread_getname_np(pthread_self(), name.data(), name.size());
    return name;
}
} // namespace aph::thread
#endif
