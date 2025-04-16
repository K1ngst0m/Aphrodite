#pragma once

#include "common/common.h"
#include "common/smallVector.h"
#include "common/timer.h"
#include <format>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

namespace aph
{

enum class BreadcrumbState
{
    Pending, // Not yet started
    InProgress, // Currently executing
    Completed, // Successfully completed
    Failed // Failed to complete
};

// Converts BreadcrumbState to a display character
inline auto StateToChar(BreadcrumbState state) -> char
{
    switch (state)
    {
    case BreadcrumbState::Pending:
        return ' ';
    case BreadcrumbState::InProgress:
        return '>';
    case BreadcrumbState::Completed:
        return 'X';
    case BreadcrumbState::Failed:
        return '!';
    default:
        return '?';
    }
}

// Converts a state to a string representation
auto StateToString(BreadcrumbState state) -> const char*;

struct Breadcrumb
{
    std::string name; // Short name of the breadcrumb
    std::string details; // Optional additional details
    BreadcrumbState state; // Current state
    uint32_t index; // Sequential index for ordering
    uint32_t depth; // Depth in the tree hierarchy
    bool isLeafNode; // Whether this is a leaf node (for indentation)

    // Timestamp tracking
    std::string startTimestamp; // Tag for start time
    std::string endTimestamp; // Tag for end time (if completed)
};

class BreadcrumbTracker
{
public:
    explicit BreadcrumbTracker(bool enabled = true, std::string name = "BreadcrumbTracker")
        : m_enabled(enabled)
        , m_name(std::move(name))
    {
    }

    ~BreadcrumbTracker() = default;

    // Add a new breadcrumb to the tree
    auto addBreadcrumb(const std::string& name, const std::string& details = "", uint32_t parentIndex = UINT32_MAX,
                       bool isLeafNode = false) -> uint32_t;

    // Update the state of an existing breadcrumb
    void updateBreadcrumb(uint32_t index, BreadcrumbState state);

    // Find a breadcrumb by name (returns UINT32_MAX if not found)
    auto findBreadcrumb(const std::string& name) const -> uint32_t;

    // Get the state of a breadcrumb by index
    auto getBreadcrumbState(uint32_t index) const -> BreadcrumbState;

    // Mark all in-progress breadcrumbs as completed
    void completeAll();

    // Reset the breadcrumb tracker
    void clear();

    // Enable or disable tracking
    void setEnabled(bool enabled);
    auto isEnabled() const -> bool;

    // Generate a formatted string representation of the breadcrumb tree
    auto toString(const std::string& header = "") const -> std::string;

    // Provides a snapshot report of breadcrumbs with state counts
    auto generateSummaryReport() const -> std::string;

    // Dumps the current breadcrumb state to the logger
    void logCurrentState(bool includeDetails = false) const;

    // Format a specific breadcrumb section with timing information
    auto formatSection(uint32_t startIndex, uint32_t endIndex = UINT32_MAX) const -> std::string;

    // Find the parent breadcrumb index for a given child index
    auto findParentIndex(uint32_t childIndex) const -> uint32_t;

    // Get access to the internal breadcrumbs collection
    auto getBreadcrumbs() const -> const SmallVector<Breadcrumb>&;

private:
    bool m_enabled = true;
    std::string m_name;
    SmallVector<Breadcrumb> m_breadcrumbs;
    uint32_t m_nextIndex = 0;
    Timer m_timer;
    mutable std::mutex m_mutex;
};

} // namespace aph
