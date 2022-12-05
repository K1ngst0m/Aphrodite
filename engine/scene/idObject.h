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
    IdType mId;

protected:
    void _setId(IdType newId) { mId = newId; }

public:
    IdObject(IdType id) : mId(id) {}

    IdType getId() const { return mId; }

    bool operator()(const IdObject *left, const IdObject *right) { return left->mId < right->mId; }

    bool operator()(const IdObject &left, const IdObject &right) { return left.mId < right.mId; }
};
}  // namespace vkl

#endif  // IDOBJECT_H_
