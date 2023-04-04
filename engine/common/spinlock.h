#ifndef SPINLOCK_H_
#define SPINLOCK_H_

#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>

namespace aph {
class SpinLock {
public:
    void Lock() {
        auto callingThread = std::this_thread::get_id();
        if (callingThread == m_owningThread)
            return;

        while (true) {
            bool value = m_lock.test_and_set(std::memory_order_acquire);
            if (!value) {
                m_owningThread = std::this_thread::get_id();
                break;
            }
        }
    }

    void Unlock() {
        m_lock.clear(std::memory_order_release);
        m_owningThread = std::thread::id();
    }

private:
    alignas(64) std::atomic_flag m_lock = ATOMIC_FLAG_INIT;
    std::thread::id m_owningThread      = std::thread::id();
};
#define BEGIN_THREAD_SAFE_BLOCK()      \
    {                                  \
        static vez::SpinLock spinLock; \
        spinLock.Lock();

#define END_THREAD_SAFE_BLOCK() \
    spinLock.Unlock();          \
    }

} // namespace aph

#endif // SPINLOCK_H_
