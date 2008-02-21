/*********************************************************
 * Copyright (C) 1998 VMware, Inc. All rights reserved.
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
 * systemLinux.c --
 *
 *    System-specific routines for all guest applications.
 *
 *    Linux implementation
 *
 */

#ifndef VMX86_DEVEL

#endif


#if !defined(__linux__) && !defined(__FreeBSD__) && !defined(sun)
#   error This file should not be compiled
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/times.h>
#include <netdb.h>
#ifdef sun
# include <sys/sockio.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
/* <netinet/in.h> must precede <arpa/in.h> for FreeBSD to compile. */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/ioctl.h>

#ifdef __FreeBSD__
#include "ifaddrs.h"
#endif

#include "vm_assert.h"
#include "system.h"
#include "debug.h"

#define MAX_IFACES      4
#define LOOPBACK        "lo"
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif


/*
 * System_Uptime --
 *
 *    Retrieve the time (in hundredth of s.) since the system has started.
 *
 *    Note: On 32-bit Linux, whether you read /proc/uptime (2 system calls: seek(2)
 *          and read(2)) or times(2) (1 system call), the uptime information
 *          comes from the 'jiffies' kernel variable, whose type is 'unsigned
 *          long'. This means that on a ix86 with HZ == 100, it will wrap after
 *          497 days. This function can detect the wrapping and still return
 *          a correct, monotonic, 64 bit wide value if it is called at least
 *          once every 497 days.
 *      
 * Result:
 *    The value on success
 *    -1 on failure (never happens in this implementation)
 *
 * Side effects:
 *    None
 *
 */

uint64
System_Uptime(void)
{
   /*
    * Dummy variable b/c times(NULL) segfaults on FreeBSD 3.2 --greg
    */
   struct tms tp; 

#if !defined (VM_X86_64)
   static uint64 base = 0;
   static unsigned long last = 0;
   uint32  current;


   ASSERT(sizeof(current) == sizeof(clock_t));

   current = times(&tp);     // 100ths of a second

   if (current < last) {
      /* The 'jiffies' kernel variable wrapped */
      base += (uint64)1 << (sizeof(current) * 8);
   }

   return base + (last = current);
#else  // VM_X86_64

   return times(&tp);
#endif
}


/*
 *----------------------------------------------------------------------
 *
 * System_GetCurrentTime --
 *
 *      Get the time in seconds & microseconds since XXX from
 *      the guest OS.
 *
 * Results:
 *      
 *      TRUE/FALSE: success/failure
 *
 * Side effects:
 *
 *	None.
 *
 *----------------------------------------------------------------------
 */

Bool
System_GetCurrentTime(int64 *secs,  // OUT
                      int64 *usecs) // OUT
{
   struct timeval tv;

   ASSERT(secs);
   ASSERT(usecs);
   
   if (gettimeofday(&tv, NULL) < 0) {
      return FALSE;
   }

   *secs = tv.tv_sec;
   *usecs = tv.tv_usec;

   return TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * System_AddToCurrentTime --
 *
 *      Adjust the current system time by adding the given number of
 *      seconds & milliseconds.
 *
 * Results:
 *      
 *      TRUE/FALSE: success/failure
 *
 * Side effects:
 *
 *	None.
 *
 *----------------------------------------------------------------------
 */

Bool
System_AddToCurrentTime(int64 deltaSecs,  // IN
                        int64 deltaUsecs) // IN
{
   struct timeval tv;
   int64 secs;
   int64 usecs;
   int64 newTime;
   
   if (!System_GetCurrentTime(&secs, &usecs)) {
      return FALSE;
   }
   
   newTime = (secs + deltaSecs) * 1000000L + (usecs + deltaUsecs);
   ASSERT(newTime > 0);
   
   /*
    * timeval.tv_sec is a 32-bit signed integer. So, Linux will treat
    * newTime as a time before the epoch if newTime is a time 68 years 
    * after the epoch (beacuse of overflow). 
    *
    * If it is a 64-bit linux, everything should be fine. 
    */
   if (sizeof tv.tv_sec < 8 && newTime / 1000000L > MAX_INT32) {
      Log("System_AddToCurrentTime() overflow: deltaSecs=%"FMT64"d, secs=%"FMT64"d\n",
          deltaSecs, secs);

      return FALSE;
   }
 
   tv.tv_sec = newTime / 1000000L;
   tv.tv_usec = newTime % 1000000L;

   if (settimeofday(&tv, NULL) < 0) {
      return FALSE;
   }
   
   return TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * System_IsACPI --
 *
 *    Is this an ACPI system?
 *
 * Results:
 *      
 *    TRUE if this is an ACPI system.
 *    FALSE if this is not an ACPI system.   
 *
 * Side effects:
 *
 *	None.
 *
 *----------------------------------------------------------------------
 */

Bool
System_IsACPI(void)
{
   ASSERT(FALSE);

   return FALSE;
}


/*
 *-----------------------------------------------------------------------------
 *  
 * System_Shutdown -- 
 *
 *   Initiate system shutdown.
 * 
 * Return value: 
 *    None.
 * 
 * Side effects:
 *    None.
 *
 *-----------------------------------------------------------------------------
 */

void
System_Shutdown(Bool reboot)  // IN: "reboot or shutdown" flag
{
   static char *cmd;

   if (reboot) {
      cmd = "shutdown -r now";
   } else {
#if __FreeBSD__
      cmd = "shutdown -p now";
#else
      cmd = "shutdown -h now";
#endif
   }
   system(cmd);
}



/*
 *----------------------------------------------------------------------
 *
 * System_GetEnv --
 *
 *
 * Results:
 *    A copy of the string
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

char *
System_GetEnv(Bool global,       // IN
              char *valueName)   // IN
{
   char *result;
   
#if defined(sun)
   result = NULL;
#else
   result = getenv(valueName);
#endif

   if (NULL != result) {
      result = strdup(result);
   }

   return(result);
} // System_GetEnv


/*
 *----------------------------------------------------------------------
 *
 * System_SetEnv --
 *
 *    Write environment variables.
 *
 *    On Linux, this only affects the local process. The global flag
 *    is ignored.
 *
 * Results:
 *    0 if success, -1 otherwise.
 *
 * Side effects:
 *    Changes the environment variable.
 *
 *----------------------------------------------------------------------
 */

int
System_SetEnv(Bool global,      // IN
              char *valueName,  // IN
              char *value)      // IN
{
#if defined(sun)
   return(-1);
#else
   return(setenv(valueName, value, 1));
#endif
} // System_SetEnv

