#ifndef APH_THREAD_UTILS_H
#define APH_THREAD_UTILS_H
#include <pthread.h>

namespace aph::thread
{
inline void setName(std::string_view name)
{
    pthread_setname_np(pthread_self(), name.data());
}

inline std::string getName()
{
    std::string name;
    name.resize(256);
    pthread_getname_np(pthread_self(), name.data(), name.size());
    return name;
}
}  // namespace aph::thread
#endif
