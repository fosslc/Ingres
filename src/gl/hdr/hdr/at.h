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
**/

/* Name: AT_ATOMIC_INCREMENT_I4
**
** Description:
**
**     Perform an Atomic Increment returning the value of the data
*/
#ifdef VMS

# define AT_ATOMIC_INCREMENT_I4( s ) __ATOMIC_INCREMENT_LONG( &(s) )

#else

  /* Currently only VMS has been defined for Atomic increments, cause an
  ** error on other platforms if used.
  */
# define AT_ATOMIC_INCREMENT_I4( s ) ATOMIC INCREMENT I4 IS NOT DEFINED FOR THIS PLATFORM

#endif

/* Other Atomic functions to be added as necessary */

# endif /* AT_HDR_INCLUDED */
