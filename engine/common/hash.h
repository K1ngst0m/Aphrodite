#ifndef APH_HASH_H_
#define APH_HASH_H_

#include "ankerl/unordered_dense.h"

namespace aph
{
#if 1
template <class Key, class T, class Hash = ::ankerl::unordered_dense::hash<Key>, class KeyEqual = std::equal_to<Key>,
          class AllocatorOrContainer = std::allocator<std::pair<Key, T>>,
          class Bucket               = ::ankerl::unordered_dense::bucket_type::standard>
using HashMap = ::ankerl::unordered_dense::map<Key, T, Hash, KeyEqual, AllocatorOrContainer, Bucket>;

template <class Key, class Hash = ::ankerl::unordered_dense::hash<Key>, class KeyEqual = std::equal_to<Key>,
          class AllocatorOrContainer = std::allocator<Key>,
          class Bucket               = ::ankerl::unordered_dense::bucket_type::standard>
using HashSet = ::ankerl::unordered_dense::set<Key, Hash, KeyEqual, AllocatorOrContainer, Bucket>;
#else
template <class Key, class Hash>
using HashMap = std::unordered_map<Key, Hash>;
template <class Key>
using HashSet = std::unordered_set<Key>;
#endif
}  // namespace aph

#endif
