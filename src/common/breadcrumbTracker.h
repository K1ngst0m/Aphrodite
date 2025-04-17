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

    auto isEnabled() const -> bool;
    void setEnabled(bool enabled);
    void clear();

    // Breadcrumb management
    auto addBreadcrumb(const std::string& name, const std::string& details = "", uint32_t parentIndex = UINT32_MAX,
                       bool isLeafNode = false) -> uint32_t;
    void updateBreadcrumb(uint32_t index, BreadcrumbState state);
    auto findBreadcrumb(const std::string& name) const -> uint32_t;
    auto getBreadcrumbState(uint32_t index) const -> BreadcrumbState;
    void completeAll();

    // Tree structure operations
    auto findParentIndex(uint32_t childIndex) const -> uint32_t;
    auto getBreadcrumbs() const -> const SmallVector<Breadcrumb>&;

    // Reporting and formatting
    auto toString(const std::string& header = "") const -> std::string;
    auto generateSummaryReport() const -> std::string;
    void logCurrentState(bool includeDetails = false) const;
    auto formatSection(uint32_t startIndex, uint32_t endIndex = UINT32_MAX) const -> std::string;

private:
    bool m_enabled = true;
    std::string m_name;
    SmallVector<Breadcrumb> m_breadcrumbs;
    uint32_t m_nextIndex = 0;
    Timer m_timer;
    mutable std::mutex m_mutex;
};

} // namespace aph
