/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<lo.h>
# include	<ooclass.h>
# include	<cu.h>
# include	<er.h>
# include	"ercu.h"
# include	<ex.h>

/*{
** Name:	cuconflict.qc	-	Deal with CUCONFLICT structure
**
** Description:
**	This files contains the routines which deal with CUCONFLICT
**	structures.
**
** History:
**	4-aug-1987 (Joe)
**		Initial Version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Update function declarations to fix gcc 4.3 problems.
*/

FUNC_EXTERN bool cu_gbfexist();

/*{
** Name:	IICUscsSetConflictState	- Set the state of a CUCONFLICT
**
** Description:
**	This routines takes the return value from a users conflict
**	routine (a bit mask of CUREPLACE CUALL CUQUIT)
**	and sets the cucheck and cureplace fields of the CUCONFLICT.
**
** Inputs:
**	rval		The return value from the user's conflict
**			routine.
**
** Outputs:
**	conflict	Sets that fields of the this structure to the
**			approriate values.
**
** History:
**	5-aug-1987 (Joe)
**		Initial Version
**	15-jan-1996 (toumi01; from 1.1 axp_osf port)
**		Cast (long) 3rd... EXsignal args to avoid truncation.
*/
IICUscsSetConflictState(rval, conflict)
i4		rval;
CUCONFLICT	*conflict;
{
    if (rval & CUQUIT)
    {
	EXsignal(EXFEBUG, 1, (long)CUQUIT);
    }
    conflict->cucheck = (rval & CUALL) != 0 ? FALSE : TRUE;
    conflict->cureplace = (rval & CUREPLACE) != 0 ? TRUE : FALSE;
}

/*{
** Name:	IICUicsInitConflictState -  Initialize conflict.
**
** Description:
**	This routine initializes a conflict structure set up by the user.
**	The conflict structure's cuwhentochk and cuconflict must be
**	set by the user.  If the cuconflict is NULL, the default
**	actions will be done.
**
** Inputs:
**	conflict	The conflict structure to initialize.
**
** Outputs:
**	conflict	Sets the fields in conflict.
**
** History:
**	5-aug-1987 (Joe)
**		Initial version.
*/
VOID
IICUicsInitConflictState(conflict)
CUCONFLICT	*conflict;
{
    if (conflict->cuwhen != CUFOREACH && conflict->cuwhen != CUATSTART)
	conflict->cuwhen = CUFOREACH;
    conflict->cucheck = TRUE;
    conflict->cureplace = FALSE;
    /*
    ** If there is no user function to call on a conflict,
    ** don't check the conflicts.
    */
    if (conflict->cuconflict == NULL)
	conflict->cucheck = FALSE;
}

/*{
** Name:	cu_conflict	- Check a file for conflicts.
**
** Description:
**	This routine will run through a copy utility input file and
**	check to see if any of the objects conflict with existing
**	objects in the database.  If they do, and the conflict structure
**	says to check it calls the user's conflict routine to determine what
**	to do.
**
** Inputs:
**	filename	The name of the file to check.
**
**	conflict	A conflict structure.
**
** Outputs:
**	conflict	The state of the conflict structure will be
**			changed.
** History:
**	5-aug-1987 (Joe)
**		Initial Version.
*/
STATUS
cu_conflict(filename, conflict)
char		*filename;
CUCONFLICT	*conflict;
{
    char	inbuf[CU_LINEMAX];
    i4		class;
    FILE	*fd;
    LOCATION	loc;
    i4		kind;
    i4		rectype;
    char	*name;
    OOID	id;

    FUNC_EXTERN char	*cu_gettok();

    if (conflict->cuwhen != CUATSTART || !conflict->cucheck)
	return OK;

    /* Open intermediate file */
    LOfroms(PATH & FILENAME, filename, &loc);
    if (SIopen(&loc, ERx("r"), &fd) != OK)
    {
	IIUGerr(E_CU0002_CANNOT_READ_FILE, 0, 1, filename);
	return E_CU0002_CANNOT_READ_FILE;
    }

    SIgetrec(inbuf, CU_LINEMAX, fd);
    for (;;)
    {	/* read until end of file reached */
	if (SIgetrec(inbuf, CU_LINEMAX, fd) == ENDFILE)
	    break;		/* end of file */
	/*
	** If we aren't supposed to check, break out since it
	** we don't check, cucheck will never change.
	*/
	if (!conflict->cucheck)
	    break;

	/* only the hdr rec types contain object names to check */
	rectype = cu_rectype(inbuf);
	kind = cu_kindof(rectype);
	switch (kind)
	{
	  case CU_OBJECT:
	    cu_gettok(inbuf);
	    _VOID_ cu_gettok(NULL);
	    name = cu_gettok(NULL);
	    if (cu_objexist(rectype, name, &id))
	    {
		IICUscsSetConflictState((*conflict->cuconflict)(rectype, name),
					conflict);
	    }
	    break;

	  case CU_GBFSTYLE:
	    cu_gettok(inbuf);
	    _VOID_ cu_gettok(NULL);
	    name = cu_gettok(NULL);
	    if (cu_gbfexist(rectype, name, cu_gettok(NULL)))
	    {
		IICUscsSetConflictState((*conflict->cuconflict)(rectype, name),
					conflict);
	    }
	    break;

	  default:
	    /*
	    ** Nothing to check.
	    */
	    break;
	}
    }
    SIclose(fd);
    conflict->cucheck = FALSE;
    return OK;
}
