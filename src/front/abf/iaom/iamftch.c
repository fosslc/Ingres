/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include	<compat.h>
#ifdef PCINGRES
#include	<ex.h>
#endif
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ooclass.h>
#include	<fdesc.h>
#include	<ifid.h>
#include	<iltypes.h>
#include	<ade.h>
#include	<frmalign.h>
#include	<ifrmobj.h>
#include	"iamstd.h"
#include	"iamtbl.h"
#include	"iamfrm.h"
#include	"iamtok.h"

/**
** Name:	iamftch.c - fetch frame object from database
**
** Description:
**	fetch frame object from database
**
** History:
**	Revision 5.1  86/08  bobm
**	Initial revision.
**
**	Revision 6.1  88/08  wong
**	Changed to use 'FEreqmem()'.
**
**	Revision 6.5
**	19-jun-92 (davel)
**		changes in support of a versioning scheme for older AOMFRAME
**		structures (necessitated by adding decimal datatype support).  
**		Patterned after changes developed for W4GL by Emerson.  
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

extern CODETABLE	Dec_tab [];
extern i4	Dtab_num;

/*{
** Name:	IIAMffFrmFetch - fetch frame
**
** Description:
**	Reads encoded frame from database into memory.
**
** Inputs:
**	fid	{OOID}  ID of frame to fetch.
**	tag	{u_i4}  Storage tag for allocation.
**
** Outputs:
**	rfrm	{IFRMOBJ **}  Reference to fetched frame structure.
**
** Returns:
**	{STATUS}  OK		success
**		  ILE_FIDBAD	non-existant frame
**		  ILE_FRDWR	INGRES failure
**		  ILE_NOMEM	memory allocation failure
**
** Side Effects:
**	allocates memory
**
** History:
**	08/86 (rlm) - Written.
*/

STATUS
IIAMffFrmFetch ( rfrm, fid, tag )
IFRMOBJ **rfrm;
OOID	fid;
u_i4	tag;
{
	STATUS		rc;
	AOMFRAME	afrm;

	/*
	** Call oo_decode to retrieve the AOMFRAME structure
	** (and the arrays it points to) from the database.
	** oo_decode's processing is initially driven by the "drive" table
	** specified by Dec_tab and Dtab_num.  This is the drive table
	** appropriate for the current version of the AOMFRAME structure.
	** When oo_decode decodes the feversion member of the AOMFRAME
	** structure, it will call analyze_Dtab_vers, which may ask oo_decode
	** to switch to a different drive table.
	*/
	rc = oo_decode(Dec_tab, Dtab_num, fid, tag, &afrm);

        if (rc != OK)
	{
		return rc;
	}
	/*
	** If this is an old AOMFRAME (version 1, created by pre-6.5 ABF),
	** set the new version 2 members of AOMFRAME (d_const and num_dc,
	** which oo_decode left full of garbage) to nulls (indicating
	** an empty array).
	*/
	if (afrm.feversion == 1)
	{
		afrm.d_const = NULL;
		afrm.num_dc = 0;
	}
	/* convert from the AOMFRAME to an IFRMOBJ and return */
	return iiamcfConvertFrame(&afrm, tag, rfrm);
}

/*{
** Name:	analyze_Dtab_vers - analyze version of AOMFRAME
**
** Description:
**	oo_decode, when called by IIAMffFrmFetch, will call this routine
**	(analyze_Dtab_vers) when it (oo_decode) decodes the feversion member
**	of the AOMFRAME structure.  This routine must tell oo_decode
**	what drive table is appropriate for the specified version.
**
**	Currently (for 6.5 ABF), there are only two versions of
**	of the AOMFRAME structure:  version 1 (created by pre-6.5 ABF),
**	is identical to version 2 (created by 6.5+ ABF), except that
**	it lacks the array of decimal literals at the end.
**
** Inputs:
**	vers	{i4}		version number (feversion member of AOMFRAME).
**	tbladdr	{CODETABLE **}	pointer to address of drive table passed to
**				oo_decode (IIAMffFrmFetch passes Dec_tab).
**	tblsize	{nat *}		pointer to size of drive table passed to
**				oo_decode (IIAMffFrmFetch passes Dtab_num).
**				Note that Dec_tab and Dtab_num together specify
**				a drive table appropriate for the current
**				version of the AOMFRAME structure.
**
** Outputs:
**	The contents of tbladdr and tblsize are modified to specify a drive
**	table appropriate for the specified version of the AOMFRAME structure.
**	Currently (6.5 ABF), all we do is decrement the contents of tblsize
**	(in effect discarding the last entry of the drive table)
**	if version number is 1.
**
** History:
**	19-jun-92 (davel)
**		Created to support decimal datatype; patterned
**		after analogous function developed by Emerson for W4GL.
*/

VOID
analyze_Dtab_vers( vers, tbladdr, tblsize )
i4		vers;
CODETABLE	**tbladdr;
i4		*tblsize;
{
	if (vers == 1)
	{
		--(*tblsize);
	}
}
