/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<er.h>
#include	<tm.h>
#include	<ilerror.h>
#include	<iltypes.h>
#include	<fdesc.h>
#include	<ade.h>
#include	<frmalign.h>
#include	<ifrmobj.h>
#include	<ooclass.h>
#include	"iamfrm.h"
#include	"iamtbl.h"
#include	"iamint.h"

/**
** Name:    iamwrop.c - Interpreted Frame Object Write Module.
**
** Description:
**	Contains routines to initialize, enter, and commit a write operation
**	to the DBMS for an interpreted frame object in sections.  Defines:
**
**	    IIAMwoWrtOpen()
**	    IIAMwcWrtCommit()
**	    IIAMil()
**	    IIAMstSymTab()
**	    IIAMicIConst()
**	    IIAMfcFConst()
**	    IIAMscSConst()
**	    IIAMxcXConst()	set variable-length string constants for write.
**	    IIAMdcDConst()
**	    IIAMisIndirSect()
**
** History:
**	Revision 5.1  87/10  bobm
**	Initial revision.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 8K (?) limit on number of integer constants
**		in a frame or procedure (allow up to 32K of them).
**
**	Revision 6.5
**	18-jun-92 (davel)
**		added support for decimal datatype.
**	08-mar-93 (davel)
**		Fix bug 49941 - return error if oo_encode() fails.
**	11-jan-1996 (toumi01; from 1.1 axp_osf port)
**		Added kchin's change (from 6.4) for axp_osf
**              08/31/93 (kchin)
**              Changed cast of i4 to long when ctab is assigned to i_const
**              in IIAMicIConst(). This is to overcome pointer trucation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Fix a silly compiler warning.
*/

extern CODETABLE Enc_tab [];
extern i4  Etab_num;

static STATUS _convertFrame();

/* frame structure to be filled in for write */
static AOMFRAME Frame;

static bool Uncalled = TRUE;

/*{
** Name:    IIAMwoWrtOpen - initialize write operation
**
** Description:
**	Initializes an internal structure for an eventual write following
**	calls to set up the frame piecemeal.
**
** Side Effects:
**	cancels any previous open write
*/

VOID
IIAMwoWrtOpen()
{
	Uncalled = FALSE;

	/* zero the "required" stuff too for bomb-proofing */
	Frame.stacksize = 0;
	Frame.indir = NULL;
	Frame.num_indir = 0;
	Frame.il = NULL;
	Frame.num_il = 0;
	Frame.symtab = NULL;
	Frame.num_sym = 0;
	Frame.c_const = NULL;
	Frame.num_cc = 0;
	Frame.f_const = NULL;
	Frame.num_fc = 0;
	Frame.i_const = NULL;
	Frame.num_ic = 0;
	Frame.x_const = NULL;
	Frame.num_xc = 0;
	Frame.d_const = NULL;
	Frame.num_dc = 0;

	set_align(&Frame);
}

/*{
** Name:	IIAMwcWrtCommit - write frame
**
** Description:
**	Performs saved write operation.	 ILE_CFRAME will be returned
**	if IIAMilIL and IIAMisIndirSect have not been called since the
**	IIAMwoWriteOpen call.  The other "set" calls are optional, and
**	will result in writes of zero length arrays if uncalled.
**
**	If an **IFRMOBJ pointer is passed in (actually passed as a PTR*), 
**	it will be set to point to the internal frame.
**
** Inputs:
**	fid	identifier for frame.
**	st	modification time of source file (to allow correction
**		for file-server clock discrepencies)
**
** Outputs:
**
**	framep  pointer to PTR, filled in to point to the
**		internal frame.
**
**	return:
**		OK		success
**		ILE_FIDBAD	non-existant frame
**		ILE_CFRAME	Required set calls haven't been made
**		ILE_FRDWR	INGRES failure
**
** Side Effects:
**	updates iiencoding table
*/

STATUS
IIAMwcWrtCommit ( fid, st, framep )
OOID	fid;
SYSTIME	*st;
PTR	*framep;
{
	STATUS	rc;

	STATUS	IIAMfeFrmExist();

	if (Uncalled)
		return (ILE_CFRAME);
	Uncalled = TRUE;

	/*
	** make sure we're using an existing FID.  Makes no difference
	** to the db_ output routines whether you are writing a new
	** encoding or overwriting an existing one.  Time stamp returned
	** by IIAMfeFrmExist is not needed here.
	*/
	switch (rc = IIAMfeFrmExist(fid, (SYSTIME *) NULL))
	{
	case ILE_ILMISSING:
	case OK:
		break;
	case ILE_FIDBAD:
		return (rc);
	default:
		return (ILE_FRDWR);
	}

#ifdef AOMDTRACE
	IIAMdump(NULL,ERx("FRAME STRUCTURE"),&Frame,sizeof(AOMFRAME));
	aomflush();
#endif

	if ( (rc = oo_encode(Enc_tab, Etab_num, fid, (PTR) &Frame)) == OK )
	{ /* touch the time stamp */
		fid_touch(fid,st);
	}
	else
	{
		return rc;
	}

	if ( framep != NULL )
	{
		IFRMOBJ	*frmtemp;

		/* convert the AOMFRAME to an IFRMOBJ
		** the following mish-mash of casting is required by DG
		*/
		*framep = ( (rc = _convertFrame(&Frame, &frmtemp)) != OK )
				? NULL : (PTR)frmtemp;
	}

	return rc;
}

/*{
** Name:	IIAMil - intermediate language
**
** Description:
**	sets intermediate language pointer for eventual write operation.
**
** Inputs:
**	il	IL for frame
**	len	length of IL
*/

VOID
IIAMil(il,len)
IL *il;
i4  len;
{
	Frame.il = il;
	Frame.num_il = len;
}

/*{
** Name:	IIAMstSymTab - Symbol Table
**
** Description:
**	sets symbol table pointer for eventual write operation.
**
** Inputs:
**	st	symbol table for frame
**	len	length
*/

VOID
IIAMstSymTab(st,len)
FDESCV2 *st;
i4  len;
{
	Frame.symtab = st;
	Frame.num_sym = len;
}

/*{
** Name:	IIAMicIConst - integer constants
**
** Description:
**	Sets integer constant table pointer for eventual write operation.
**	Although the frame actually expects an i4 pointer, the data given
**	to us by ILG is an array of strings, which is how we will interpret
**	it when we write.
**
** Inputs:
**	ctab	constant table for frame
**	len	length
*/

VOID
IIAMicIConst(ctab,len)
char **ctab;
i4  len;
{
	Frame.i_const = (i4 *) ctab;
	Frame.num_ic = len;
}

/*{
** Name:	IIAMfcFConst - floating constants
**
** Description:
**	Sets floating constant table pointer for eventual write operation.
**
** Inputs:
**	ctab	constant table for frame
**	len	length
*/

VOID
IIAMfcFConst(ctab,len)
char **ctab;
i4  len;
{
	Frame.f_const = ctab;
	Frame.num_fc = len;
}

/*{
** Name:	IIAMscSConst - string constants
**
** Description:
**	Sets string constant table pointer for eventual write operation.
**
** Inputs:
**	ctab	constant table for frame
**	len	length
*/

VOID
IIAMscSConst(ctab,len)
char **ctab;
i4  len;
{
	Frame.c_const = ctab;
	Frame.num_cc = len;
}

/*{
** Name:	IIAMxcXConst - Variable-Length String Constants.
**
** Description:
**	Sets variable-length string constant table pointer for eventual
**	write operation.  Although the frame actually expects a pointer to
**	a DB_TEXT_STRING and the data passed in by ILG is an array of pointers
**	to strings that are actually references to DB_TEXT_STRINGS.
**
** Inputs:
**	ctab	{char **}  constant table for frame
**	len	{nat}  length
*/

VOID
IIAMxcXConst (ctab, len)
char **ctab;
i4  len;
{
	Frame.x_const = (DB_TEXT_STRING **)ctab;
	Frame.num_xc = len;
}

/*{
** Name:      IIAMdcDConst - decimal constants
**
** Description:
**    Sets decimal constant table pointer for eventual write operation.
**
** Inputs:
**    ctab    constant table for frame
**    len     length
*/

VOID
IIAMdcDConst(ctab,len)
char **ctab;
i4  len;
{
	Frame.d_const = ctab;
	Frame.num_dc = len;
}

/*{
** Name:	IIAMisIndirSect - stack indirection section
**
** Description:
**	sets stack indirection section and stack size for eventual
**	frame write.
**
** Inputs:
**	indir	IL_DB_VDESC's - stack indirection section
**	len	length
**	totsize total stack size
*/

VOID
IIAMisIndirSect(indir,len,totsize)
IL_DB_VDESC *indir;
i4  len;
i4  totsize;
{
	Frame.stacksize = totsize;
	Frame.indir = indir;
	Frame.num_indir = len;
}
/*{
** Name:	_convertFrame - take an AOMFRAME and produce an IFRMOBJ
**
** Description:
**	Calls iiamcfConvertFrame to do most of the work of creating an
**	IFRMOBJ from an AOMFRAME.  This routine then has to process the 
**	integer list, since the AOMFRAME produced by translation has a list of
**	pointers to string representations of ints (hack attack!) and
**	the AOMFRAME from the DBMS has true ints.  There are good reasons
**	for this cheat, really there are.
**
** Inputs:
**	afrm	{AOMFRAME *}  dbms frame to convert
**
** Outputs:
**	rfrm	{IFRMOBJ **}  Reference to fetched frame structure.
**
** Returns:
**	{STATUS}  OK		success
**		  ILE_NOMEM	memory allocation failure
**
** Side Effects:
**	allocates memory
**
** History:
**	2/6/89 - firsst written (billc)
*/

static STATUS
_convertFrame (afrm, rfrm)
AOMFRAME *afrm;
IFRMOBJ **rfrm;
{
	IFRMOBJ *frm;
	i4	i;
	u_i4	tag; 
	STATUS		rc;
	STATUS  iiamcfConvertFrame();

	tag = FEgettag();

	/* do the generic conversion. */
	if ((rc = iiamcfConvertFrame(afrm, tag, rfrm)) != OK)
		return rc;
	frm = *rfrm;

	/* This is the in-core version of the AOMFRAME struct, so the integer 
	** list is a list of string pointers, and needs to be converted.
	*/

	frm->num_ic = afrm->num_ic;
	if ( afrm->num_ic == 0 )
	{
		frm->i_const = NULL;
	}
        else 
	{
		if ( (frm->i_const = (i4 *)FEreqlng(tag,
		    (u_i4)sizeof(i4) * afrm->num_ic, FALSE, &rc)) == NULL )
		{
			return rc;
		}
		for ( i = 0 ; i < frm->num_ic ; ++i )
		{
			CVal(((PTR *)(afrm->i_const))[i], &(frm->i_const[i]));
		}
	}

	return OK;
}

