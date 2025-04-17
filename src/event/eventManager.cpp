#include "eventManager.h"

void aph::EventManager::processAll()
{
    for (auto& [_, handler] : m_eventDataMap)
    {
        handler->process();
    }
}

auto aph::EventManager::nextTypeID() -> TypeID
{
    static TypeID next = 0;
    return next++;
}
