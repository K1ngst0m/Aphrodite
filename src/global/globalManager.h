#pragma once

#include "common/enum.h"
#include "common/hash.h"

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
    static constexpr const char* FILESYSTEM_NAME = "Filesystem";
    static constexpr const char* EVENT_MANAGER_NAME = "EventManger";
    static constexpr const char* MEMORY_TRACKER_NAME = "MemoryTracker";

    /**
     * @brief Enumeration of priority levels for initialization and shutdown
     * 
     * Higher priority components are initialized EARLIER and destroyed LATER
     * This ensures dependencies are properly managed
     */
    enum class InitPriority : int32_t
    {
        Lowest = 0,      // Last to initialize, first to destroy
        Low = 25,        // Normal application components
        Normal = 50,     // Default for most subsystems
        High = 75,       // Core engine systems
        Highest = 100    // Critical systems (memory, logging)
    };

public:
    /**
     * @brief Available built-in subsystems
     */
    enum class BuiltInSystemBits : uint32_t
    {
        None = 0,
        TaskManager = (1 << 0),
        Filesystem = (1 << 1),
        EventManager = (1 << 2),
        MemoryTracker = (1 << 3),

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
     * 
     * Systems will be shut down in reverse priority order (lowest priority first)
     */
    void shutdown();

    /**
     * @brief Template method to register a custom subsystem
     * @tparam T Type of the subsystem
     * @param name Unique identifier for the subsystem
     * @param system Pointer to the subsystem instance
     * @param priority Priority level for initialization/shutdown order
     * @return True if registration succeeded, false if already exists
     */
    template <typename T>
    bool registerSubsystem(std::string_view name, std::unique_ptr<T> system, 
                          InitPriority priority = InitPriority::Normal);

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

    // Information about a subsystem
    struct SubsystemInfo
    {
        std::string name;
        InitPriority priority;
        
        // For sorting 
        bool operator<(const SubsystemInfo& other) const {
            // Sort by priority in descending order
            return static_cast<int32_t>(priority) > static_cast<int32_t>(other.priority);
        }
    };

    // Container for custom subsystems with type-safe deletion
    using TypeErasedPtr = std::unique_ptr<void, std::function<void(void*)>>;
    HashMap<std::string, TypeErasedPtr> m_subsystems;
    
    // Track initialization order for orderly shutdown
    std::vector<SubsystemInfo> m_initOrder;

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
bool GlobalManager::registerSubsystem(std::string_view name, std::unique_ptr<T> system, InitPriority priority)
{
    std::string nameStr{ name };
    if (m_subsystems.find(nameStr) != m_subsystems.end())
    {
        return false;
    }

    // Create a type-erased unique_ptr with a custom deleter
    auto* rawPtr = system.release();
    std::function<void(void*)> deleter = [](void* ptr) { delete static_cast<T*>(ptr); };

    // Store in the map with proper type information for deletion
    m_subsystems.emplace(nameStr, TypeErasedPtr{ rawPtr, deleter });
    
    // Record the initialization order with priority
    m_initOrder.push_back({nameStr, priority});
    
    // Sort the initialization order after each addition
    std::sort(m_initOrder.begin(), m_initOrder.end());
    
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
    (*::aph::getGlobalManager().getSubsystem<aph::TaskManager>(aph::GlobalManager::TASK_MANAGER_NAME))
#define APH_DEFAULT_FILESYSTEM \
    (*::aph::getGlobalManager().getSubsystem<aph::Filesystem>(aph::GlobalManager::FILESYSTEM_NAME))
#define APH_DEFAULT_EVENT_MANAGER \
    (*::aph::getGlobalManager().getSubsystem<aph::EventManager>(aph::GlobalManager::EVENT_MANAGER_NAME))
#define APH_MEMORY_TRACKER \
    (*::aph::getGlobalManager().getSubsystem<aph::memory::AllocationTracker>(aph::GlobalManager::MEMORY_TRACKER_NAME))
