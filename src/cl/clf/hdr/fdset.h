/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**	(Author and creation date unknown)
**
**	17-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Force error if <clconfig.h> has not been not included;
**		Define new macros AIX_FD_SET, AIX_FD_CLR, AIX_FD_ISSET, for
**		AIX style select structure.
**	15-aug-1990 (chsieh)
**		Added bu2_us5 and bu3_us5 specific code to include 
**              <sys/select.h>.
**	21-mar-1991 (seng)
**		Add <sys/select.h> for AIX 3.1
**      09-feb-1995 (chech02)
**           Added rs4_us5 for AIX 4.1
**      29-aug-1997 (musro02)
**           Add sqs_ptx to platforms that need sys/select.h
**	26-Nov-1997 (allmi01)
**	     Added sgi_us5 to list of platforms that need to include 
**	     <sys/select.h>.
**	10-may-1999 (walro03)
**	    Remove obsolete version string bu2_us5, bu3_us5.
**	17-may-1999 (hweho01)
**	    Added ris_u64 to list of platforms that need <sys/select.h>. 
**	19-apr-1999 (hanch04)
**	    Change long to int.
**	19-jul-2001 (stephenb)
**	    Add support for i64_aix
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**
*/

# ifndef CLCONFIG_H_INCLUDED
        # error "didn't include clconfig.h before fdset.h"
# endif

/* Temporary; these are defined in Sun OS 4.0 <sys/types.h> */

/* Some systems don't have fd_set defined, e.g AIX. So we need to define 
   fd_set here. */

#if defined(any_aix) || defined(sqs_ptx) || defined(sgi_us5)
#include <sys/select.h>
#endif

#ifndef xCL_014_FD_SET_TYPE_EXISTS

#define	NBBY	8		/* number of bits in a byte */
#ifndef	FD_SETSIZE
#define	FD_SETSIZE	256
#endif

typedef int	fd_mask;
#define NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */
#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif

typedef	struct fd_set {
	fd_mask	fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	MEfill(sizeof(*(p)),'\0',(char *)(p))

#else /* xCL_014_FD_SET_TYPE_EXISTS */

#ifndef FD_SETSIZE

#define	FD_SETSIZE	32
#define	NFDBITS		32

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	MEfill(sizeof(*(p)),'\0',(char *)(p))

#endif
#endif /* xCL_014_FD_SET_TYPE_EXISTS */

# ifdef xCL_021_SELSTRUCT
#define AIX_FD_SET(n, p)        ((p)->fdsmask[(n)/NFDBITS] |= (1 << ((n) % NFDBI
TS)))
#define AIX_FD_CLR(n, p)        ((p)->fdsmask[(n)/NFDBITS] &= ~(1 << ((n) % NFDB
ITS)))
#define AIX_FD_ISSET(n, p)      ((p)->fdsmask[(n)/NFDBITS] & (1 << ((n) % NFDBIT
S)))
# endif /* xCL_021_SELSTRUCT */

