/*
** Copyright (c) 2010 Ingres Corporation
*/
# ifndef AT_HDR_INCLUDED
# define AT_HDR_INCLUDED

#ifdef VMS

#include <builtins.h>

#endif

/**
** Name:	AT.h	- Define Atomic functions
**
** Specification:
**
** Description:
**	Define native Atomic functions. An Atomic function is a
**      function that will successfully complete its operation
**      if there is no interference from another thread. Where
**      interferences occurs, no change in state will occur and the
**      operation will be repeated. Interference implies another
**      thread simultaneously attempt to read/write the data item.
**
** History:
**	1-jun-2010 (horda03)
**	    Created
**      14-Jun-2010 (horda03) B123919
**          Add AT_ATOMIC_INCREMENT_I4 for linux.
**/

/* Name: AT_ATOMIC_INCREMENT_I4
**
** Description:
**
**     Perform an Atomic Increment returning the value of the data
*/
#ifdef VMS

# define AT_ATOMIC_INCREMENT_I4( s ) __ATOMIC_INCREMENT_LONG( &(s) )

#elif defined(int_lnx) || defined(a64_lnx)

# define AT_ATOMIC_INCREMENT_I4( s ) at_lnx_increment_i4( &(s), 1 )

static __inline__ i4
at_lnx_increment_i4( i4 *s, i4 add )
{
   /* add "add" to "*s". After operation
   ** "add" holds the original value of *s
   */
   __asm__ __volatile__(
          "lock ; xaddl %0, %1"
          : "+r" (add), "+m" (*s)
          : : "memory");

   return  add;
}


#else

  /* Currently only VMS has been defined for Atomic increments, cause an
  ** error on other platforms if used.
  */
# define AT_ATOMIC_INCREMENT_I4( s ) ATOMIC INCREMENT I4 IS NOT DEFINED FOR THIS PLATFORM

#endif

/* Other Atomic functions to be added as necessary */

# endif /* AT_HDR_INCLUDED */
