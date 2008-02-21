/*********************************************************
 * Copyright (C) 2006 VMware, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation version 2.1 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA.
 *
 *********************************************************/

/*
 * stub.c --
 *
 *   Stub for unuseful functions. Stolen from the Tools upgrader.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "str.h"
#include "vm_assert.h"


/*
 *----------------------------------------------------------------------
 *
 * StubVprintf --
 *
 *      Output error text.
 *
 * Results:
 *      
 *      None.
 *
 * Side effects:
 *
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
StubVprintf(const char *prefix,
            const char *fmt,
            va_list args)
{
   char *str = Str_Vasprintf(NULL, fmt, args);
   if (str) {
      fprintf(stderr, "%s: %s", prefix, str);
      free(str);
   } else {
      fprintf(stderr, "%s: out of memory in panic!", prefix);
   }
}


/*
 *----------------------------------------------------------------------
 *
 * Panic --
 *
 *      Mmmm...Panic.
 *
 * Results:
 *      
 *      None.
 *
 * Side effects:
 *
 *	Kills the program.
 *
 *----------------------------------------------------------------------
 */

void
Panic(const char *fmt,...)
{
   va_list args;

   va_start(args, fmt);
   StubVprintf("PANIC", fmt, args);
   va_end(args);

   exit(255);
   NOT_REACHED();
}
