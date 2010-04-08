/*
**	Copyright (c) 1983 Ingres Corporation
**
**	Name:
**		pclocal.h
**
**	Function:
**		None
**
**	Arguments:
**		None
**
**	Result:
**		defines symbols for PC module routines
**
**	Side Effects:
**		None
**
**	History:
**		3/82 (gb) written
**	31-Oct-1988 (daveb)
**		Remove PIDQUEUE definition and vfork
**	25-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Force error if <clconfig.h> has not been included.
**	14-may-95 (emmag)
**	    DESKTOP porting changes.
**	27-may-97 (mcgem01)
**	    Cleaned up compiler warnings.
**      18-jun-97 (harpa06)
**          Added PCw95cmdline()
**      30-Oct-97 (fanra01)
**          Removed the need for PCw95cmdline.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**/

# ifndef CLCONFIG_H_INCLUDED
	# error "didn't include clconfig.h before pclocal.h (in cl/clf/pc)"
# endif

/*
	functions specific to BSD and system V
*/

# ifndef xCL_029_EXECVP_EXISTS
# define	execvp		execv
# define	execlp		execl
# endif

/*
	used by UNIX system call access()
*/

# define	READ		 4
# define	WRITE		 2
# define	EXECUTE		 1
# define	ACCESSIBLE	 0
# define	BAD_ACCESS	-1

/*
	UNIX system global error number variable
*/

# ifdef DESKTOP
extern		int		errnum;	/* it's supposed to be an int   */
# else
extern		int		errno;	/* it's supposed to be an int   */
# endif //DESKTOP



/*
	stores current STATUS of PC routines
*/

extern		STATUS		PCstatus;
FUNC_EXTERN	STATUS PCwntcmdline(
#ifdef CL_PROTOTYPED
	LOCATION *      interpreter,
	char *          cmdline,
	i4              wait,
	i4              append,
	LOCATION *      err_log,
	CL_ERR_DESC *   err_code
#endif
);
