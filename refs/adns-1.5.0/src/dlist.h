/*
 * dlist.h
 * - macros for handling doubly linked lists
 */
/*
 *  This file is part of adns, which is
 *    Copyright (C) 1997-2000,2003,2006,2014  Ian Jackson
 *    Copyright (C) 2014  Mark Wooding
 *    Copyright (C) 1999-2000,2003,2006  Tony Finch
 *    Copyright (C) 1991 Massachusetts Institute of Technology
 *  (See the file INSTALL for full details.)
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3, or (at your option)
 *  any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation.
 */

#ifndef ADNS_DLIST_H_INCLUDED
#define ADNS_DLIST_H_INCLUDED

#define LIST_INIT(list) ((list).head= (list).tail= 0)
#define LINK_INIT(link) ((link).next= (link).back= 0)

#define LIST_UNLINK_PART(list,node,part)				    \
  do {									    \
    if ((node)->part back) (node)->part back->part next= (node)->part next; \
      else                                  (list).head= (node)->part next; \
    if ((node)->part next) (node)->part next->part back= (node)->part back; \
      else                                  (list).tail= (node)->part back; \
  } while(0)

#define LIST_LINK_TAIL_PART(list,node,part)		\
  do {							\
    (node)->part next= 0;				\
    (node)->part back= (list).tail;			\
    if ((list).tail) (list).tail->part next= (node);	\
    else (list).head= (node);				\
    (list).tail= (node);				\
  } while(0)

#define LIST_UNLINK(list,node) LIST_UNLINK_PART(list,node,)
#define LIST_LINK_TAIL(list,node) LIST_LINK_TAIL_PART(list,node,)

#endif
