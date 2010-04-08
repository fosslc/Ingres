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

extern		int		errno;	/* it's supposed to be an int   */



/*
	stores current STATUS of PC routines
*/

extern		STATUS		PCstatus;
