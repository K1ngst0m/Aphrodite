#include "globalManager.h"

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

void GlobalManager::initialize(BuiltInSystemFlags systems)
{
    // Initialize TaskManager if requested
    if (systems & BuiltInSystemBits::TaskManager)
    {
        auto taskManager = std::make_unique<TaskManager>();
        registerSubsystem<TaskManager>(TASK_MANAGER_NAME, std::move(taskManager));
    }

    // Add initialization for other built-in subsystems here
    //
    m_init.store(true);
}

void GlobalManager::shutdown()
{
    m_subsystems.clear();
}

} // namespace aph
