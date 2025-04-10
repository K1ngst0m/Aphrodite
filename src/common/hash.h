#pragma once

#include "ankerl/unordered_dense.h"

namespace aph
{
template <class Key, class T, class Hash = ::ankerl::unordered_dense::hash<Key>, class KeyEqual = std::equal_to<Key>,
          class AllocatorOrContainer = std::allocator<std::pair<Key, T>>,
          class Bucket               = ::ankerl::unordered_dense::bucket_type::standard>
using HashMap = ::ankerl::unordered_dense::map<Key, T, Hash, KeyEqual, AllocatorOrContainer, Bucket>;

template <class Key, class Hash = ::ankerl::unordered_dense::hash<Key>, class KeyEqual = std::equal_to<Key>,
          class AllocatorOrContainer = std::allocator<Key>,
          class Bucket               = ::ankerl::unordered_dense::bucket_type::standard>
using HashSet = ::ankerl::unordered_dense::set<Key, Hash, KeyEqual, AllocatorOrContainer, Bucket>;
} // namespace aph
