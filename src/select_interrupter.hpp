//
// detail/select_interrupter.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef XXSOCKET_SELECT_INTERRUPTER_HPP
#define XXSOCKET_SELECT_INTERRUPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)


#if !defined(_XXSOCKET_INLINE)
#define _XXSOCKET_INLINE inline
#endif

#if defined(_WIN32) 
# include "socket_select_interrupter.hpp"
#elif defined(__linux__)
# include "eventfd_select_interrupter.hpp"
#else
# include "pipe_select_interrupter.hpp"
#endif

namespace purelib {
namespace inet {

#if defined(_WIN32) 
typedef socket_select_interrupter select_interrupter;
#elif defined(__linux__)
typedef eventfd_select_interrupter select_interrupter;
#else
typedef pipe_select_interrupter select_interrupter;
#endif

} // namespace inet
} // namespace purelib

#endif // BOOST_ASIO_DETAIL_SELECT_INTERRUPTER_HPP
