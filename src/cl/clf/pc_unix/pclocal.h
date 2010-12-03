/*
**	Copyright (c) 1998, 2004 Ingres Corporation
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
**      20-may-1997 (canor01)
**          If II_PCCMDPROC is set to a process name, use it as a command
**          process slave.  PCcmdline() will pass all commands to it.
**	    Structures used by the command process slave are defined here.
**	27-jan-1998 (canor01)
**	    II_PCCMDPROC changes are specific to Jasmine.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	15-nov-2010 (stephenb)
**	    Add defined from redundant local copy of pc.h (now removed) and
**	    correctly prototype local functions.
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

# define	EXEC		01	/* access mode for executing */

# define	AREAD		0444	/* all read access */
# define	AWRITE		0222	/* all write access */
# define	AEXEC		0111	/* all execute access */


# define	OREAD		0400	/* owner read access */
# define	OWRITE		0200	/* owner write access */
# define	OEXEC		0100	/* owner execute access */

/*
**	needed as UNIX doesn't check world status
**		if user is in same group as owner???
*/

# define	WREAD		0044	/* group and world read access */
# define	WWRITE		0022	/* group and world write access */
# define	WEXEC		0011	/* group and world execute access */

# define	UNKNOWN		0	/* unknown permission status */
# define	GIVE		1	/* give permission status */
# define	TAKE		2	/* take away permission status */

# define	UMASKLENGTH	6	/* length of the umask */
# define	PEBUFSZ		256	/* string buffer size in PE.c */

# define	NONE		0	/* resets the permission bits */


/*
	UNIX system global error number variable
*/

extern		int		errno;	/* it's supposed to be an int   */



/*
	stores current STATUS of PC routines
*/

extern		STATUS		PCstatus;

/* local functions */
STATUS	PCsspawn(i4, char **, bool, PIPE *, PIPE *, PID *);
STATUS	PCno_fork(void);
STATUS	PCno_exec(char *);
STATUS 	PCdospawn(i4, char **, bool, LOCATION *, LOCATION *, i4, i4, PID *);
STATUS	PCsendex(EX, PID);
STATUS	PCgetexecname(char *argv0, char *buf);
