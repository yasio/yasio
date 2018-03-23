# aclocal.m4 - package-specific macros for autoconf
#  
#  This file is part of adns, which is
#    Copyright (C) 1997-2000,2003,2006,2014  Ian Jackson
#    Copyright (C) 2014  Mark Wooding
#    Copyright (C) 1999-2000,2003,2006  Tony Finch
#    Copyright (C) 1991 Massachusetts Institute of Technology
#  (See the file INSTALL for full details.)
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3, or (at your option)
#  any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation.

dnl DPKG_CACHED_TRY_COMPILE(<description>,<cachevar>,<include>,<program>,<ifyes>,<ifno>)
define(DPKG_CACHED_TRY_COMPILE,[
 AC_MSG_CHECKING($1)
 AC_CACHE_VAL($2,[
  AC_TRY_COMPILE([$3],[$4],[$2=yes],[$2=no])
 ])
 if test "x$$2" = xyes; then
  true
  $5
 else
  true
  $6
 fi
])

define(ADNS_C_GCCATTRIB,[
 DPKG_CACHED_TRY_COMPILE(__attribute__((,,)),adns_cv_c_attribute_supported,,
  [extern int testfunction(int x) __attribute__((,,))],
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_GNUC25_ATTRIB)
   DPKG_CACHED_TRY_COMPILE(__attribute__((noreturn)),adns_cv_c_attribute_noreturn,,
    [extern int testfunction(int x) __attribute__((noreturn))],
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_GNUC25_NORETURN),
    AC_MSG_RESULT(no))
   DPKG_CACHED_TRY_COMPILE(__attribute__((const)),adns_cv_c_attribute_const,,
    [extern int testfunction(int x) __attribute__((const))],
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_GNUC25_CONST),
    AC_MSG_RESULT(no))
   DPKG_CACHED_TRY_COMPILE(__attribute__((format...)),adns_cv_attribute_format,,
    [extern int testfunction(char *y, ...) __attribute__((format(printf,1,2)))],
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_GNUC25_PRINTFFORMAT),
    AC_MSG_RESULT(no)),
  AC_MSG_RESULT(no))
])

define(ADNS_C_GETFUNC,[
 AC_CHECK_FUNC([$1],,[
  AC_CHECK_LIB([$2],[$1],[$3],[
    AC_MSG_ERROR([cannot find library function $1])
  ])
 ])
])
