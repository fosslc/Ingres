/*
**  Copyright (c) 1986, 2004 Ingres Corporation
*/
# include       <compat.h>
# include	<si.h>
# include       <st.h>
# include       <gl.h>
# include       <iicommon.h>
# include       <fe.h>
# include	<ooclass.h>
# include	<oocat.h>
# include	<abfdbcat.h>
# include	<cu.h>
# include	<ug.h>
# include	"erie.h"
# include       "ieimpexp.h"

/**
** Name: iedcomps.sc
** 
** Defines:
**	IIIEdoc_DeleteOldComponent()	- delete an existing catalog entry
**
** History:
**	18-aug-1993 (daver)
**		First Written/Documented
**	14-sep-1993 (daver)
**		Moved error message over to erie.msg, random cleanup shme.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

EXEC SQL INCLUDE SQLCA;
EXEC SQL WHENEVER SQLERROR CALL SQLPRINT;


/*{
** Name: IIIEdoc_DeleteOldComponent      - delete existing catalog entry
**
** Description:
**	This routine is needed to remove entries in catalogs where an
**	object is described via multiple rows. When the catalogs use a 1:1,
**	normalized mapping between rows and objects, we can simply update those
**	rows. But when multiple rows are stored per object, we need to delete
**	them all, and insert the new ones.
**
** Input:
**	IEOBJREC	*objrec		pointer to object (and id) to delete.
**	i4		appid		application id of object
**	i4		comptype	type of component it is
**
** Output:
**	None
**
** Returns:
**	Whatever the status of the query in question returns.
**
** Side Effects:
**	Deletes the specified component from the appropriate catalog.
**
** History:
**	18-aug-1993 (daver)
**		First Written.
**	14-sep-1993 (daver)
**		Moved error message over to erie.msg, random cleanup shme.
*/
STATUS
IIIEdoc_DeleteOldComponent(objrec, appid, comptype)
IEOBJREC	*objrec;
EXEC SQL BEGIN DECLARE SECTION;
i4		appid;	
EXEC SQL END DECLARE SECTION;
i4		comptype;
{

	EXEC SQL BEGIN DECLARE SECTION;
	i4	objid;
	EXEC SQL END DECLARE SECTION;

	if (IEDebug)
		SIprintf("In DeleteOldComponent, deleting %s's %s...\n\n",
		objrec->name,IICUrtnRectypeToName(comptype));

	objid = objrec->id;

	switch (comptype)
	{
	  case CUC_AODEPEND:
	  case CUC_ADEPEND:
		/*
		** Don't delete the ILCODE dependencies that this 
		** object has (class = 2010) or the Menu dependencies
		** (dependency type = 3505) -- the menu deps indicate that
		** this frame is a vision frame and has child frames beneath it
		** in the frame flow diagram.
		**
		** For frames, this will delete the formref dependencies,
		** which is a good thing, since they will be added later, and
		** the new form associated with this frame may not necessarily
		** be the same name as the old one
		**
		** PS. Careful about using defined symbols rather than
		** the raw numbers 2010 and 3505; I've seen these changed
		** back to numbers elsewhere, claiming gateway/porting problems.
		*/
	  	EXEC SQL DELETE FROM ii_abfdependencies
		WHERE object_id = :objid
		AND abfdef_applid = :appid
		AND object_class <> 2010
		AND abfdef_deptype <> 3505; 
		break;

	  case CUC_FIELD:
		EXEC SQL DELETE FROM ii_fields
		WHERE object_id = :objid;
		break;

	  case CUC_TRIM:
		EXEC SQL DELETE FROM ii_trim
		WHERE object_id = :objid;
		break;

	  case CUC_RCOMMANDS:
		EXEC SQL DELETE FROM ii_rcommands
		WHERE object_id = :objid;
		break;

	  case CUC_VQJOIN:
		EXEC SQL DELETE FROM ii_vqjoins
		WHERE object_id = :objid;
		break;

	  case CUC_VQTAB:
		EXEC SQL DELETE FROM ii_vqtables
		WHERE object_id = :objid;
		break;

	  case CUC_VQTCOL:
		EXEC SQL DELETE FROM ii_vqtabcols
		WHERE object_id = :objid;
		break;

	  case CUC_MENUARG:
		EXEC SQL DELETE FROM ii_menuargs
		WHERE object_id = :objid;
		break;
	
	  case CUC_FRMVAR:
		EXEC SQL DELETE FROM ii_framevars
		WHERE object_id = :objid;
		break;

	  case CUC_ESCAPE:
		EXEC SQL DELETE FROM ii_encodings
		WHERE object_id = :objid;
		break;
	
	  default:
		IIUGerr(E_IE0029_Unknown_Component, UG_ERR_ERROR, 2,
			(PTR)objrec->name, (PTR)comptype);

		return (CUQUIT);
		
	} /* end switch */

	return(FEinqerr()); 
}
