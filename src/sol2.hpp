#pragma once

#if defined(_WIN32)
#include <yvals.h>
#endif

#if (defined(_HAS_CXX17) && _HAS_CXX17) || __cplusplus >= 201703L
#if !defined(_HAS_CXX17)
#define _HAS_CXX17 1
#endif
#else
#if !defined(_HAS_CXX17)
#define _HAS_CXX17 0
#endif
#endif

#if _HAS_CXX17
#include "sol.hpp"
#else
#include "sol11.hpp"
#endif

