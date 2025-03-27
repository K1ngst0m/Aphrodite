#pragma once

#include "api/gpuResource.h"
#include "common.h"

namespace aph
{
class DataBuilder
{
public:
    explicit DataBuilder(uint32_t minAlignment) noexcept
        : m_minAlignment(minAlignment)
    {
        APH_ASSERT(minAlignment != 0 && (minAlignment & (minAlignment - 1)) == 0 &&
                   "minAlignment must be a power of 2!");
    }

    template <typename T_Data>
        requires std::is_trivially_copyable_v<T_Data> && (!std::is_pointer_v<T_Data>)
    void writeTo(T_Data& writePtr) noexcept
    {
        writeTo(&writePtr);
    }

    void writeTo(const void* writePtr) noexcept
    {
        std::memcpy((uint8_t*)writePtr, getData().data(), getData().size());
    }

    const std::vector<std::byte>& getData() const noexcept
    {
        return m_data;
    }

    std::vector<std::byte>& getData() noexcept
    {
        return m_data;
    }

    void reset() noexcept
    {
        m_data.clear();
    }

    template <typename T_Data>
    uint32_t addRange(T_Data dataRange, Range range = {})
    {
        if (range.size == 0)
        {
            range.size = sizeof(T_Data);
        }

        static_assert(std::is_trivially_copyable_v<T_Data>, "The range data must be trivially copyable");
        const std::byte* rawDataPtr = reinterpret_cast<const std::byte*>(&dataRange);

        APH_ASSERT(range.offset + range.size <= sizeof(T_Data));

        size_t bytesToCopy = range.size;

        uint32_t offset = aph::utils::paddingSize(m_minAlignment, m_data.size());

        size_t newSize = offset + bytesToCopy;

        CM_LOG_DEBUG("addrange: offset: %u, range_offset: %u, bytesToCopy: %u, m_data.size(): %u", offset, range.offset,
                     bytesToCopy, m_data.size());

        if (newSize > m_data.size())
        {
            m_data.resize(newSize);
        }

        std::memcpy(m_data.data() + offset, rawDataPtr + range.offset, bytesToCopy);

        return offset;
    }

private:
    std::vector<std::byte> m_data;
    uint32_t m_minAlignment;
};
} // namespace aph
