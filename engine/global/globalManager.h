#pragma once

#include "common/enum.h"
#include "threads/taskManager.h"
#include <functional>

namespace aph
{
/**
 * @brief Central hub for managing global components
 *
 * GlobalManager provides factory pattern methods to create and access
 * singleton objects.
 */
class GlobalManager
{
public:
    // Names for built-in subsystems
    static constexpr const char* TASK_MANAGER_NAME = "TaskManager";

public:
    /**
     * @brief Available built-in subsystems
     */
    enum class BuiltInSystemBits : uint32_t
    {
        None = 0,
        TaskManager = (1 << 0),

        // Add other built-in systems here with bit flags
        // Example: RenderSystem = (1 << 1),

        All = 0xFFFFFFFF
    };
    using BuiltInSystemFlags = Flags<BuiltInSystemBits>;

    /**
     * @brief Get the singleton instance of the GlobalManager
     * @return Reference to the GlobalManager instance
     */
    static GlobalManager& instance();

    /**
     * @brief Initialize the GlobalManager with selected built-in systems
     * @param systems Flags specifying which systems to initialize
     */
    void initialize(BuiltInSystemFlags systems = BuiltInSystemBits::All);

    /**
     * @brief Shutdown all subsystems and cleanup resources
     */
    void shutdown();

    /**
     * @brief Template method to register a custom subsystem
     * @tparam T Type of the subsystem
     * @param name Unique identifier for the subsystem
     * @param system Pointer to the subsystem instance
     * @return True if registration succeeded, false if already exists
     */
    template <typename T>
    bool registerSubsystem(std::string_view name, std::unique_ptr<T> system);

    /**
     * @brief Template method to retrieve a registered subsystem
     * @tparam T Type of the subsystem to retrieve
     * @param name Unique identifier of the subsystem
     * @return Pointer to the subsystem instance, or nullptr if not found
     */
    template <typename T>
    T* getSubsystem(std::string_view name);

private:
    GlobalManager() = default;
    ~GlobalManager() = default;

    GlobalManager(const GlobalManager&) = delete;
    GlobalManager& operator=(const GlobalManager&) = delete;
    GlobalManager(GlobalManager&&) = delete;
    GlobalManager& operator=(GlobalManager&&) = delete;

    // Container for custom subsystems with type-safe deletion
    using TypeErasedPtr = std::unique_ptr<void, std::function<void(void*)>>;
    HashMap<std::string, TypeErasedPtr> m_subsystems;

    std::atomic<bool> m_init = false;
};

// Define FlagTraits for BuiltInSystemBits to enable bitwise operations
template <>
struct FlagTraits<GlobalManager::BuiltInSystemBits>
{
    static constexpr bool isBitmask = true;
    static constexpr GlobalManager::BuiltInSystemFlags allFlags = GlobalManager::BuiltInSystemBits::All;
};

template <typename T>
bool GlobalManager::registerSubsystem(std::string_view name, std::unique_ptr<T> system)
{
    std::string nameStr{ name };
    if (m_subsystems.find(nameStr) != m_subsystems.end())
    {
        CM_LOG_WARN("the system %s has already registered.");
        return false;
    }

    // Create a type-erased unique_ptr with a custom deleter
    auto* rawPtr = system.release();
    std::function<void(void*)> deleter = [](void* ptr) { delete static_cast<T*>(ptr); };

    // Store in the map with proper type information for deletion
    m_subsystems.emplace(nameStr, TypeErasedPtr{ rawPtr, deleter });
    return true;
}

template <typename T>
T* GlobalManager::getSubsystem(std::string_view name)
{
    std::string nameStr(name);
    if (auto it = m_subsystems.find(nameStr); it != m_subsystems.end())
    {
        // Cast back to the original type
        return static_cast<T*>(it->second.get());
    }
    return nullptr;
}

inline GlobalManager& getGlobalManager()
{
    return GlobalManager::instance();
}

} // namespace aph

#define APH_GLOBAL_MANAGER ::aph::getGlobalManager()
#define APH_DEFAULT_TASK_MANAGER \
    *::aph::getGlobalManager().getSubsystem<aph::TaskManager>(aph::GlobalManager::TASK_MANAGER_NAME)
