#pragma once

#include "common/common.h"

namespace aph
{

typedef uint32_t IdType;

class Id
{
public:
    template <typename T>
    static IdType generateNewId()
    {
        static IdType g_currentId = 0;
        return g_currentId++;
    }
};

class IdObject
{
public:
    IdObject(IdType id)
        : m_Id(id)
    {
    }
    virtual ~IdObject() = default;

    IdType getId() const
    {
        return m_Id;
    }

private:
    IdType m_Id;
};
} // namespace aph
