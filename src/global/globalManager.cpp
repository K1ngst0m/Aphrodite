#include "globalManager.h"

#include "allocator/allocator.h"
#include "common/logger.h"
#include "event/eventManager.h"
#include "filesystem/filesystem.h"
#include "threads/taskManager.h"

namespace aph
{

GlobalManager& GlobalManager::instance()
{
    static GlobalManager s_instance;

    static std::once_flag s_initFlag;
    std::call_once(s_initFlag,
                   []()
                   {
                       if (s_instance.m_init)
                       {
                           return;
                       }
                       // Auto-initialize with default configuration
                       s_instance.initialize();
                   });

    return s_instance;
}

GlobalManager::~GlobalManager()
{
    shutdown();
}

void GlobalManager::initialize(BuiltInSystemFlags systems)
{
    // Collect the subsystems to initialize based on flags
    SmallVector<std::pair<std::string, std::pair<std::function<void()>, InitPriority>>> subsystemsToInit;

    // Initialize Logger if requested - highest priority (initialized first, destroyed last)
    if (systems & BuiltInSystemBits::Logger)
    {
        subsystemsToInit.push_back({
            LOGGER_NAME,
            {[this]()
             {
                 auto logger = std::make_unique<Logger>();
                 logger->initialize(); // Initialize the logger

                 registerSubsystem<Logger>(LOGGER_NAME, std::move(logger),
                                           InitPriority::Highest // Logger needs highest priority
                 );
             }, InitPriority::Highest}
        });
    }

    // Initialize MemoryTracker if requested - highest priority (initialized first, destroyed last)
    if (systems & BuiltInSystemBits::MemoryTracker)
    {
        subsystemsToInit.push_back({
            MEMORY_TRACKER_NAME,
            {[this]()
             {
                 auto memoryTracker = std::make_unique<memory::AllocationTracker>();

                 // Register with automatic report generation on shutdown
                 registerSubsystem<memory::AllocationTracker>(
                     MEMORY_TRACKER_NAME, std::move(memoryTracker),
                     InitPriority::Highest, // Memory tracker needs highest priority
                     [this]()
                     {
                         std::string report = APH_MEMORY_TRACKER.generateSummaryReport();
                         if (auto logger = getSubsystem<Logger>(LOGGER_NAME))
                         {
                             logger->debug("Memory Tracker Final Report: %s", report.c_str());
                             logger->flush();
                         }
                     });
             }, InitPriority::Highest}
        });
    }

    // Initialize Filesystem if requested - high priority (core system)
    if (systems & BuiltInSystemBits::Filesystem)
    {
        subsystemsToInit.push_back({
            FILESYSTEM_NAME,
            {[this]()
             {
                 auto filesystem = std::make_unique<Filesystem>();
                 registerSubsystem<Filesystem>(FILESYSTEM_NAME, std::move(filesystem),
                                               InitPriority::High // File system is a core dependency
                 );
             }, InitPriority::High}
        });
    }

    // Initialize TaskManager if requested - normal priority
    if (systems & BuiltInSystemBits::TaskManager)
    {
        subsystemsToInit.push_back({
            TASK_MANAGER_NAME,
            {
              [this]()
                {
                    auto taskManager = std::make_unique<TaskManager>();
                    registerSubsystem<TaskManager>(TASK_MANAGER_NAME, std::move(taskManager),
                                                   InitPriority::Normal // Standard subsystem
                    );
                }, InitPriority::Normal,
              }
        });
    }

    // Initialize EventManager if requested - low priority
    if (systems & BuiltInSystemBits::EventManager)
    {
        subsystemsToInit.push_back({
            EVENT_MANAGER_NAME,
            {[this]()
             {
                 auto eventManager = std::make_unique<EventManager>();
                 registerSubsystem<EventManager>(EVENT_MANAGER_NAME, std::move(eventManager),
                                                 InitPriority::Low // Depends on other subsystems
                 );
             }, InitPriority::Low}
        });
    }

    // Sort by priority (high priority initialized first)
    std::sort(subsystemsToInit.begin(), subsystemsToInit.end(), [](const auto& a, const auto& b)
              { return static_cast<int32_t>(a.second.second) > static_cast<int32_t>(b.second.second); });

    // Initialize in priority order
    for (const auto& [name, initPair] : subsystemsToInit)
    {
        initPair.first(); // Execute the initialization function
    }

    m_init.store(true);
}

bool GlobalManager::registerShutdownCallback(std::string_view name, ShutdownCallback callback)
{
    if (!callback)
    {
        return false;
    }

    std::string nameStr{name};

    // Check if the subsystem exists
    if (m_subsystems.find(nameStr) == m_subsystems.end())
    {
        return false;
    }

    // Find the subsystem in our initialization order and add the callback
    for (auto& info : m_initOrder)
    {
        if (info.name == nameStr)
        {
            info.shutdownCallback = std::move(callback);
            return true;
        }
    }

    return false;
}

void GlobalManager::shutdown()
{
    // Get a copy of the initialization order for shutdown
    auto shutdownOrder = m_initOrder;
    std::reverse(shutdownOrder.begin(), shutdownOrder.end());

    // Systems will be shut down in reverse priority order (lowest priority first)
    // This is already handled by the sorting method in the SubsystemInfo class

    // Shutdown each system in the correct order
    for (const auto& sysInfo : shutdownOrder)
    {
        if (auto it = m_subsystems.find(sysInfo.name); it != m_subsystems.end())
        {
            CM_LOG_INFO("Shutting down Global Instance: %s", sysInfo.name);
            // Execute the callback before removing the system
            if (sysInfo.shutdownCallback)
            {
                sysInfo.shutdownCallback();
            }

            // Remove the system
            m_subsystems.erase(it);
        }
    }

    // Clear the initialization record
    m_initOrder.clear();
}

} // namespace aph
