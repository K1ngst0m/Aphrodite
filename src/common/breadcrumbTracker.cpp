#include "common/breadcrumbTracker.h"
#include "common/logger.h"

namespace aph
{

// Define a namespace-scope logger macro for breadcrumb system
GENERATE_LOG_FUNCS(BCT);

// Converts a state to a string representation
auto StateToString(BreadcrumbState state) -> const char*
{
    switch (state)
    {
    case BreadcrumbState::Pending:
        return "Pending";
    case BreadcrumbState::InProgress:
        return "InProgress";
    case BreadcrumbState::Completed:
        return "Completed";
    case BreadcrumbState::Failed:
        return "Failed";
    default:
        return "Unknown";
    }
}

// Add a new breadcrumb to the tree
auto BreadcrumbTracker::addBreadcrumb(const std::string& name, const std::string& details, uint32_t parentIndex,
                                      bool isLeafNode) -> uint32_t
{
    if (!m_enabled)
        return UINT32_MAX;

    std::lock_guard<std::mutex> lock(m_mutex);

    uint32_t depth = 0;
    if (parentIndex != UINT32_MAX)
    {
        for (const auto& crumb : m_breadcrumbs)
        {
            if (crumb.index == parentIndex)
            {
                depth = crumb.depth + 1;
                break;
            }
        }
    }

    // Create timestamp tag
    std::string timestampTag = std::format("event_{}", m_nextIndex);
    m_timer.set(timestampTag);

    m_breadcrumbs.push_back({ .name           = name,
                              .details        = details,
                              .state          = BreadcrumbState::InProgress,
                              .index          = m_nextIndex,
                              .depth          = depth,
                              .isLeafNode     = isLeafNode,
                              .startTimestamp = timestampTag,
                              .endTimestamp   = {} });

    return m_nextIndex++;
}

// Find the parent breadcrumb index for a given child index
auto BreadcrumbTracker::findParentIndex(uint32_t childIndex) const -> uint32_t
{
    if (!m_enabled)
        return UINT32_MAX;

    std::lock_guard<std::mutex> lock(m_mutex);

    uint32_t childDepth = 0;
    bool found          = false;

    // Find the depth of the child
    for (const auto& crumb : m_breadcrumbs)
    {
        if (crumb.index == childIndex)
        {
            childDepth = crumb.depth;
            found      = true;
            break;
        }
    }

    if (!found || childDepth == 0)
        return UINT32_MAX;

    // Find the parent (a breadcrumb with depth = childDepth - 1 that comes before the child)
    for (auto it = m_breadcrumbs.rbegin(); it != m_breadcrumbs.rend(); ++it)
    {
        if (it->index < childIndex && it->depth == childDepth - 1)
        {
            return it->index;
        }
    }

    return UINT32_MAX;
}

// Get access to the internal breadcrumbs collection
auto BreadcrumbTracker::getBreadcrumbs() const -> const SmallVector<Breadcrumb>&
{
    return m_breadcrumbs;
}

// Update the state of an existing breadcrumb
void BreadcrumbTracker::updateBreadcrumb(uint32_t index, BreadcrumbState state)
{
    if (!m_enabled)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& crumb : m_breadcrumbs)
    {
        if (crumb.index == index)
        {
            crumb.state = state;

            // If completing or failing, record end timestamp
            if (state == BreadcrumbState::Completed || state == BreadcrumbState::Failed)
            {
                std::string endTimestamp = std::format("event_{}_end", index);
                m_timer.set(endTimestamp);
                crumb.endTimestamp = endTimestamp;
            }
            break;
        }
    }
}

// Reset the breadcrumb tracker
void BreadcrumbTracker::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_breadcrumbs.clear();
    m_nextIndex = 0;
}

// Enable or disable tracking
void BreadcrumbTracker::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (enabled == false)
    {
        clear();
    }
}

auto BreadcrumbTracker::isEnabled() const -> bool
{
    return m_enabled;
}

// Generate a formatted string representation of the breadcrumb tree
auto BreadcrumbTracker::toString(const std::string& header) const -> std::string
{
    if (!m_enabled || m_breadcrumbs.empty())
    {
        return "No breadcrumbs recorded";
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    std::stringstream ss;

    // Add header if provided, otherwise use the default
    if (!header.empty())
    {
        ss << header << "\n";
    }
    else if (!m_name.empty())
    {
        ss << m_name << " Breadcrumbs:\n";
    }

    // Calculate column width for the duration values
    constexpr int durationColWidth = 8;

    for (const auto& crumb : m_breadcrumbs)
    {
        // Format duration information
        std::string durationStr = "        ";
        if (!crumb.endTimestamp.empty())
        {
            double duration = m_timer.interval(crumb.startTimestamp, crumb.endTimestamp);
            // Convert to milliseconds
            duration *= 1000.0;
            durationStr = std::format("[{:.3f}ms]", duration);
        }

        // Add state indicator and indentation
        ss << '[' << StateToChar(crumb.state) << "] ";

        // Add indentation based on depth
        for (uint32_t i = 0; i < crumb.depth; ++i)
        {
            // Check if this is the last element at its level
            if (i == crumb.depth - 1)
            {
                ss << (crumb.isLeafNode ? "└─" : "├─");
            }
            else
            {
                ss << "│ ";
            }
        }

        // Add name with quotes and details if present
        ss << '"' << crumb.name << '"';
        if (!crumb.details.empty())
        {
            ss << " (" << crumb.details << ")";
        }

        ss << '\n';
    }

    return ss.str();
}

// Format a specific breadcrumb section with timing information
auto BreadcrumbTracker::formatSection(uint32_t startIndex, uint32_t endIndex) const -> std::string
{
    if (!m_enabled || m_breadcrumbs.empty())
    {
        return "No breadcrumbs recorded";
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    std::stringstream ss;

    uint32_t actualEndIndex = (endIndex == UINT32_MAX) ? m_nextIndex - 1 : endIndex;

    for (const auto& crumb : m_breadcrumbs)
    {
        if (crumb.index < startIndex || crumb.index > actualEndIndex)
            continue;

        // Format as in toString but with more timing details if available
        ss << '[' << StateToChar(crumb.state) << "] ";

        for (uint32_t i = 0; i < crumb.depth; ++i)
        {
            if (i == crumb.depth - 1)
            {
                ss << (crumb.isLeafNode ? "└─" : "├─");
            }
            else
            {
                ss << "│ ";
            }
        }

        ss << '"' << crumb.name << '"';

        // Add timing information if completed
        if (!crumb.endTimestamp.empty())
        {
            double duration = m_timer.interval(crumb.startTimestamp, crumb.endTimestamp);
            duration *= 1000.0; // Convert to milliseconds
            ss << std::format(" [{:.3f}ms]", duration);
        }

        if (!crumb.details.empty())
        {
            ss << " (" << crumb.details << ")";
        }

        ss << '\n';
    }

    return ss.str();
}

// Provides a snapshot report of breadcrumbs with state counts
auto BreadcrumbTracker::generateSummaryReport() const -> std::string
{
    if (!m_enabled || m_breadcrumbs.empty())
    {
        return "No breadcrumbs recorded";
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // Count breadcrumbs by state
    uint32_t pendingCount    = 0;
    uint32_t inProgressCount = 0;
    uint32_t completedCount  = 0;
    uint32_t failedCount     = 0;

    for (const auto& crumb : m_breadcrumbs)
    {
        switch (crumb.state)
        {
        case BreadcrumbState::Pending:
            pendingCount++;
            break;
        case BreadcrumbState::InProgress:
            inProgressCount++;
            break;
        case BreadcrumbState::Completed:
            completedCount++;
            break;
        case BreadcrumbState::Failed:
            failedCount++;
            break;
        }
    }

    // Build the summary report
    std::stringstream ss;
    ss << m_name << " Summary Report:\n"
       << "===========================================\n"
       << "Total breadcrumbs: " << m_breadcrumbs.size() << "\n"
       << "  Pending: " << pendingCount << "\n"
       << "  In Progress: " << inProgressCount << "\n"
       << "  Completed: " << completedCount << "\n"
       << "  Failed: " << failedCount << "\n"
       << "===========================================\n";

    // If failures exist, list them
    if (failedCount > 0)
    {
        ss << "\nFailed Breadcrumbs:\n"
           << "-------------------------------------------\n";

        for (const auto& crumb : m_breadcrumbs)
        {
            if (crumb.state == BreadcrumbState::Failed)
            {
                ss << " - \"" << crumb.name << "\"";
                if (!crumb.details.empty())
                {
                    ss << " (" << crumb.details << ")";
                }
                ss << "\n";
            }
        }
        ss << "-------------------------------------------\n";
    }

    return ss.str();
}

// Dumps the current breadcrumb state to the logger
void BreadcrumbTracker::logCurrentState(bool includeDetails) const
{
    if (!m_enabled || m_breadcrumbs.empty())
    {
        BCT_LOG_INFO("No breadcrumbs recorded");
        return;
    }

    BCT_LOG_INFO("%s", toString().c_str());

    if (includeDetails)
    {
        BCT_LOG_INFO("%s", generateSummaryReport().c_str());
    }
}

// Find a breadcrumb by name (returns UINT32_MAX if not found)
auto BreadcrumbTracker::findBreadcrumb(const std::string& name) const -> uint32_t
{
    if (!m_enabled)
        return UINT32_MAX;

    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& crumb : m_breadcrumbs)
    {
        if (crumb.name == name)
        {
            return crumb.index;
        }
    }

    return UINT32_MAX;
}

// Get the state of a breadcrumb by index
auto BreadcrumbTracker::getBreadcrumbState(uint32_t index) const -> BreadcrumbState
{
    if (!m_enabled)
        return BreadcrumbState::Pending;

    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& crumb : m_breadcrumbs)
    {
        if (crumb.index == index)
        {
            return crumb.state;
        }
    }

    return BreadcrumbState::Pending;
}

// Mark all in-progress breadcrumbs as completed
void BreadcrumbTracker::completeAll()
{
    if (!m_enabled)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& crumb : m_breadcrumbs)
    {
        if (crumb.state == BreadcrumbState::InProgress)
        {
            crumb.state = BreadcrumbState::Completed;

            // Record end timestamp
            std::string endTimestamp = std::format("event_{}_end", crumb.index);
            m_timer.set(endTimestamp);
            crumb.endTimestamp = endTimestamp;
        }
    }
}

} // namespace aph