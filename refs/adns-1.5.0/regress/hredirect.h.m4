m4_dnl hredirect.h.m4
m4_dnl (part of complex test harness, not of the library)
m4_dnl - redefinitions of system calls

m4_dnl  This file is part of adns, which is
m4_dnl    Copyright (C) 1997-2000,2003,2006,2014  Ian Jackson
m4_dnl    Copyright (C) 2014  Mark Wooding
m4_dnl    Copyright (C) 1999-2000,2003,2006  Tony Finch
m4_dnl    Copyright (C) 1991 Massachusetts Institute of Technology
m4_dnl  (See the file INSTALL for full details.)
m4_dnl  
m4_dnl  This program is free software; you can redistribute it and/or modify
m4_dnl  it under the terms of the GNU General Public License as published by
m4_dnl  the Free Software Foundation; either version 3, or (at your option)
m4_dnl  any later version.
m4_dnl  
m4_dnl  This program is distributed in the hope that it will be useful,
m4_dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
m4_dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
m4_dnl  GNU General Public License for more details.
m4_dnl  
m4_dnl  You should have received a copy of the GNU General Public License
m4_dnl  along with this program; if not, write to the Free Software Foundation.

m4_include(hmacros.i4)

#ifndef HREDIRECT_H_INCLUDED
#define HREDIRECT_H_INCLUDED

#include "hsyscalls.h"

hm_create_nothing
m4_define(`hm_syscall', `#undef $1
#define $1 H$1')
m4_define(`hm_specsyscall',`#undef $2
#define $2 H$2')
m4_include(`hsyscalls.i4')

#endif
