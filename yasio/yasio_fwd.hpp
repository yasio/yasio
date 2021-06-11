#ifndef YASIO__FWD_HPP
#define YASIO__FWD_HPP
#include "yasio/compiler/feature_test.hpp"

namespace yasio
{
YASIO__NS_INLINE
namespace inet
{
class io_service;
class io_event;
} // namespace inet
#if !YASIO__HAS_NS_INLINE
using namespace yasio::inet;
#endif
} // namespace yasio

#endif
