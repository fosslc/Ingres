/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** dup2.c -- dup2 emulation for systems without the system call
**
** History:
**    10-oct-88 (daveb)
**		Split out from pc/pcsspawn.c
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

# include <compat.h>
# include <gl.h>
# include <clconfig.h>

# ifdef xCL_012_DUP2_EXISTS

int IIusing_system_dup2;

# else

# include <fcntl.h>

/*
** As per BSD dup2(2) interface:
*/
dup2(oldd, newd)
int	oldd;
int	newd;
{
	int		fcntl();
	extern	int	errno;

	(VOID) close(newd);
	return (fcntl(oldd, F_DUPFD, newd));
}

# endif
