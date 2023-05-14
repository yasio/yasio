//////////////////////////////////////////////////////////////////////////////////////////
// A multi-platform support c++11 library with focus on asynchronous socket I/O for any
// client application.
//////////////////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 2012-2023 HALX99 (halx99 at live dot com)

#pragma once

#include "yasio/detail/config.hpp"

#if !defined(YASIO_DISABLE_POLL)
#  include "yasio/detail/poll_io_watcher.hpp"
#else
#  include "yasio/detail/select_io_watcher.hpp"
#endif

namespace yasio
{
YASIO__NS_INLINE
namespace inet
{
#if !defined(YASIO_DISABLE_POLL)
using io_watcher = poll_io_watcher;
#else
using io_watcher = select_io_watcher;
#endif
} // namespace inet
} // namespace yasio
