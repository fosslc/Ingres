/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** CLITIMER.H -- emulate itimers
**
** History:
**      27-Oct-1988 (daveb)
**          Created.
**	17-may-90 (blaise)
**	    Integrated changes from 61 and ug:
**		Force errors if <clconfig.h> and <rusage.h> are not included.
**
*/

# ifndef CLCONFIG_H_INCLUDED
        # error "didn't include clconfig.h before clitimer.h"
# endif

# ifndef MIN_PER_HR
        # error "Must include rusage.h before clitimer.h"
# endif

# ifndef xCL_030_SETITIMER_EXISTS

/* 
** Need to emulate itimers 
*/

# define getitimer(which, val)		IIgetitimer( which, val )
# define setitimer(which, val, oval)	IIsetitimer( which, val, oval )
# define itimerval IIitimerval

# define ITIMER_REAL	0	/* only one supported in emulation */

/* a struct timeval might have been defined for select, so make this
   one different, but with the same member names */

struct	IItimeval	
{ 
	int 	tv_sec;
	int	tv_usec;
};

/* This is really IIitimerval, as hidden with the define above */

struct itimerval
{
	struct IItimeval it_interval;
	struct IItimeval it_value;
};

# endif
