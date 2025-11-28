#pragma once

#include "yasio/vector.hpp"
#include "yasio/buffer_alloc.hpp"
#include <type_traits>

namespace yasio
{
// alias: array_buffer
template <typename _Ty, typename _Alloc = yasio::crt_buffer_allocator<_Ty>>
using array_buffer = typename std::enable_if<std::is_trivially_copyable<_Ty>::value, ::yasio::vector<_Ty, _Alloc>>::type;
} // namespace yasio
