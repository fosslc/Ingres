# include 	<compat.h>
# include 	<gl.h>
# include	<systypes.h>
# include	<clconfig.h>
# include	<si.h>
# include	<clnfile.h>
# include       <sys/stat.h>

/*
**Copyright (c) 2004 Ingres Corporation
 *
 *	Name:
 *		SIisopen()
 *
 *	Function:
 *		Is stream open?
 *
 *	Arguments:
 *		fp : a pointer to the file in question.
 *
 *	Result:
 *		Test whether the FILE* corresponds to an open stream.
 *		Not guaranteed to work, just tests whether pointer and 
 *		referenced values look reasonable.
 *
 *	Side Effects:
 *
 *	History:
 *	06-Feb-89 (anton)
 *	    Use _NFILE instead of constant.
 *		4/13/83 -	(mmm) written
 *	07-mar-89 (russ)
 *	    Use getdtablesize instead of _NFILE, if it exists.
 *	08-may-89 (daveb)
 *	    Use CL_NFILE() instead of _NFILE or getdtablesize.
**	29-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add new function ii_SIisopen to check whether a file descriptor
**		is open.
**	25-mar-91 (kirke)
**	    Moved #include <systypes.h> because HP-UX's unistd.h (included
**	    by clnfile.h) includes sys/types.h.
**      05-Nov-1992 (mikem)
**          su4_us5 6.5 port.  Changed direct calls to CL_NFILE to calls to new
**          function iiCL_get_fd_table_size().
**      05-Jan-1993 (mikem)
**          su4_us5 6.5 port.  Changed cast of "fileno(fp) to a i4 from
**	    an unsigned char.  This makes it work on hp/bull as well as sun.
**	05-mar-1993 (mikem) integrated following change: 26-Jan-1993 (fredv)
**          Changed cast of "fileno(fp)" to "unsigned i4" from
**          "i4". Otherwise, for those machines that:
**              o fileno(fp) is a char, and
**              o default char is signed
**          will fail when fileno(fp) is greater than 127.
**	    
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-jun-95 (sarjo01)
**	    For NT port: use GetFileType()
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-nov-2010 (stephenb)
**	    ii_SIisopen() is only used here, make it static and proto 
**	    it properly.
**/

static bool ii_SIisopen(i4);

bool
SIisopen(fp)
FILE	*fp;
{
# ifndef NT_GENERIC
    /* first do we have a valid file pointer ? */
    if ((fp == NULL) || 
        ((u_i4) fileno(fp) > ((i4) iiCL_get_fd_table_size())))
    {
	return FALSE;
    }

    /* yes..see if they are open */
    if(!ii_SIisopen(fileno(fp)))
	return FALSE;

    return TRUE;

# else

    STATUS   status;

    status = GetFileType((HANDLE)fileno(fp));

    return((status == -1) ? FALSE : TRUE);

# endif
}

static bool
ii_SIisopen(fd)
i4      fd;
{
        struct stat statbuf;

        /* see if they are open */
        if(fstat(fd, &statbuf) < 0)
                return FALSE;
        return TRUE;
}
