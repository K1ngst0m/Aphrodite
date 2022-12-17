#ifndef IDOBJECT_H_
#define IDOBJECT_H_

#include "common/common.h"

namespace vkl
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
private:
    friend struct IdCmp;  // Avoid calling getId()
    IdType m_Id;

protected:
    void _setId(IdType newId) { m_Id = newId; }

public:
    IdObject(IdType id) : m_Id(id) {}

    IdType getId() const { return m_Id; }

    bool operator()(const IdObject *left, const IdObject *right) { return left->m_Id < right->m_Id; }

    bool operator()(const IdObject &left, const IdObject &right) { return left.m_Id < right.m_Id; }
};
}  // namespace vkl

#endif  // IDOBJECT_H_
