/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <fe.h>
# include <st.h>
# include <stype.h>
# include <sglob.h>
# include <er.h>

/*
**   S_XNS_SET - Enable expanded name space - owner.tablename
**		 and delimited identifiers.
**
** Parameters:
**	None.
**
** Output:
**	St_xns_given set to TRUE.
**
** Returns:
**	TRUE -	Command valid.
**	FALSE -	Command invalid - not supported according to DB Capabilities.
**
** History:
**	23-sep-1992 (rdrane)
**		Created.
**	25-nov-1992 (rdrane)
**		Rename references to NAM_ANSI92 to NAM_DELIMID as per the LRC.
**	4-jan-1994 (rdrane)
**		Check dlm_Case capability directly to determine if
**		delimited identifiers are suppported, rather than
**		trusting the StandardCatalogLevel.
*/


bool
IISRxs_XNSSet()
{


	if (Cact_ren == NULL)
	{
	    s_error(0x38A,FATAL,NULL);
	}

	if  (IIUIdlmcase() == UI_UNDEFCASE)
	{
	    return(FALSE);
	}

	/*
	** If it's already in force,
	** treat the command as a no-op.
	*/
	if (St_xns_given)
	{ 
	    return(TRUE);
	}

	save_em();

	/* Set up fields in Command */

	STcopy(NAM_DELIMID,Ctype);
	Csequence = 0;
	STcopy(ERx(" "),Csection);
	STcopy(ERx(" "),Cattid);
	STcopy(ERx(" "),Ccommand);
	STcopy(ERx(" "),Ctext);

	_VOID_ s_w_row();

	get_em();

	St_xns_given = TRUE;

	return(TRUE);
}

