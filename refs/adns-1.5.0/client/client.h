/*
 * clients.h
 * - useful declarations and definitions for adns client programs
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

#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#define ADNS_VERSION_STRING "1.5.0"

#define COPYRIGHT_MESSAGE \
 "Copyright (C) 1997-2000,2003,2006,2014  Ian Jackson\n" \
 "Copyright (C) 2014  Mark Wooding\n" \
 "Copyright (C) 1999-2000,2003,2006  Tony Finch\n" \
 "Copyright (C) 1991 Massachusetts Institute of Technology\n" \
 "This is free software; see the source for copying conditions.  There is NO\n" \
 "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"

#define VERSION_MESSAGE(program) \
 program " (GNU adns) " ADNS_VERSION_STRING "\n\n" COPYRIGHT_MESSAGE

#define VERSION_PRINT_QUIT(program)                               \
  if (fputs(VERSION_MESSAGE(program),stdout) == EOF ||            \
      fclose(stdout)) {                                           \
    perror(program ": write version message");                    \
    quitnow(-1);                                                  \
  }                                                               \
  quitnow(0);

void quitnow(int rc) NONRETURNING;

#endif
