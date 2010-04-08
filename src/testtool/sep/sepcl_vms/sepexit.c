/*
**	Name:
**		SEPexit.c
**
**	Function:
**		SEPexit
**
**	Arguments:
**		i4	status;
**
**	Result:
**		Program exit.
**		Status is value returned to shell or command interpreter.
**
**	History:
**		??-may-1989 (eduardo)
**			Created.
**		13-sep-1989 (eduardo)
**			Use TC files to communicate exit status to listexec.
**		17-jul-1991 (donj)
**			Change SI_iob to SI_iobptr as per new CL.
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**      02-apr-1992 (donj)
**          Removed unneeded references. Changed all refs to '\0' to EOS. Chang
**          increments (c++) to CM calls (CMnext(c)). Changed a "while (*c)"
**          to the standard "while (*c != EOS)". Implement other CM routines
**          as per CM documentation.
**      09-aug-1993 (donj)
**	    Lost a change:
**		23-feb-1993 (donj)
**	          LOfroms() now translates strings into uppercase AND translates
**	          logical names, which is not good for mailbox I/O.
*/

#include <ssdef.h>
#include <compat.h>
#include <si.h>
#include <ex.h>
#include <lo.h>
#include <cv.h>
#include <cm.h>
#include <tc.h>
#include <er.h>

GLOBALREF    bool          fromListexec ;
GLOBALREF    char          listexecKey [MAX_LOC] ;

SEPexit(status)
i4 status;
{
    FILE                  *fp = SI_iobptr ;
    int                    i ;
    LOCATION               aloc ;
    TCFILE                *aptr ;
    char                   exitvalue [10] ;
    char                  *eptr ;
    char                   tmpstr [MAX_LOC+1] ;

    for (i = 0; i < _NFILE; i++)
	SIclose(fp++);

    if (fromListexec)
    {
	STcopy(listexecKey, tmpstr);
	LOfroms(FILENAME, listexecKey, &aloc);
	STcopy(tmpstr, aloc.string);
	if (TCopen(&aloc, ERx("w"), &aptr) == OK)
	{
	    eptr = exitvalue;
	    CVna(status, eptr);
	    eptr = exitvalue;

	    while (*eptr != EOS)
	    {
		TCputc(*eptr, aptr);
		CMnext(eptr);
	    }

	    TCclose(aptr);
	}
    }
    sys$exit(1);

}
