/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<lo.h>
# include	<st.h>
# include	<er.h>
# include	<nm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <ooclass.h>
# include       <adf.h>
# include       <ft.h>
# include       <fmt.h>
# include       <frame.h>
# include       <metafrm.h>
# include       <abclass.h>

/**
** Name:	fgscform.c - Save Form to database
**
** Description:
**	This file defines:
**
**	IIFGscfSaveCompForm - Save complete form
**      IIFGgtd_getTempDir  - Get Temporary Directory for form save routine
**
** History:
**	1/2/90 (Mike S) Initial version
**	08/31/90 (dkh) - Integrated porting change ingres6202p/131116.
**	9/12/90 (Mike S) Don't compile form.
**/

/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN LOCATION        *iiabMkFormLoc();
FUNC_EXTERN LOCATION        *IIFGgtd_getTempDir();

/* static's */
static LOCATION temploc;        /* Temporary location */
static bool     no_temploc = TRUE;      /* is temploc uninitialized */


/*{
** Name:	IIFGscfSaveCompForm - Save and compile a form

**
** Description:
**	Save a FRAME structure to the database and compile it into the source
**	directory for the application.
**
** Inputs:
**	apobj	USER_FRAME *	application frame
**	frame	FRAME *		Frame structure to save
**
** Outputs:
**
**	Returns:
**		STATUS	status
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	1/2/90 (Mike S)	Initial version
*/
STATUS
IIFGscfSaveCompForm(apobj, frame)
USER_FRAME 	*apobj;
FRAME		*frame;
{
        char    *shortrmk;      /* Short remarks */
        char    *longrmk;       /* long remarks */
	STATUS	status;
	
        /* Save form into database */
        IIFGmr_makeRemarks(apobj->name, apobj->appl->name, &shortrmk, &longrmk);
        status = IICFsave(frame, IIFGgtd_getTempDir(), TRUE, shortrmk, longrmk);
        if (status != OK)
		return (status);

	/* Clear change bits for fields */
        IIFGccbClearChangeBits(apobj->mf);

	return(status);
}


/*{
** Name:        IIFGgtd_getTempDir Get Temporary Directory for form save routine**
** Description:
**      Get the default temporary directory.  This is a cover for NMloc,
**      used because we're going to need this information so often.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
**      Returns:
**              LOCATION *      temporary directory
**
**      Exceptions:
**              none
**
** Side Effects:
**      none
**
** History:
**      7/14/89 (Mike S)        Initial version
*/
LOCATION *
IIFGgtd_getTempDir()
{
        /* If we haven't gotten the location yet, get it. */
        if (no_temploc)
        {
                NMloc(TEMP, PATH, NULL, &temploc);
                no_temploc = FALSE;
        }
        return(&temploc);
}
