/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	"iamstd.h"
# include	<iltypes.h>
# include	<fdesc.h>
# include	<ade.h>
# include	<frmalign.h>
# include	<ifrmobj.h>
# include	"iamtbl.h"
# include	"eram.h"

/**
** Name:	iamdtbl.c -	Decode Drive Table Definition.
**
** Description:
**	Drive table for decoding of AOMFRAME structure.
**
**	CAUTION:  This is a template which must exactly match the layout
**		of the AOMFRAME definition.
**
**	This structure defines both decoding / encoding of structure.  To
**	avoid unneeded procedure linking, 2 copies are being maintained.
**	This copy is used for the decoding operation.  The encoding procedures
**	are fatal. Structural changes will have to be reflected in the
**	encoding table also!
**
** History:
**	Revision 6.0  86/08  bobm
**	Initial revision.
**
**	Revision 6.1  88/08  wong
**	Modified to use 'FEreqmem()'.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails changing the call to the allocation procedure
**		(aproc in CODETABLE); it now expects the size to be a u_i4.
**
**	Revision 6.5
**	22-jun-92 (davel)
**		Changes in support of decimal datatype support - decode
**		new members "d_const" and "num_dc" onto the end of the
**		AOMFRAME structure; add intializers for new "vproc" member 
**		of CODETABLE.  These changes patterned after analogous
**		changes developed by Emerson for W4GL.
**	22-jul-92 (blaise)
**		Changed vproc_noop() from static VOID to VOID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

PTR	sym_alloc(); /* symbol table allocation */

/*
** decode procedures
*/
VOID	analyze_Dtab_vers(); /* analyze version number of decode drive table */
i4	oo_gdec();	/* "generic" decode routine */
i4	ind_dec();	/* stack indirection section */
i4	sym_dec();	/* symbol table */
i4	hex_dec();	/* hexadecimals */

VOID
vproc_noop( vers, tbladdr, tblsize )
i4		vers;
CODETABLE	**tbladdr;
i4		*tblsize;
{
	return;
}

static i4
derr()
{
	IIUGerr(S_AM0012_unexp_encode, UG_ERR_FATAL, 0);
}

static PTR
talloc ( tag, nbytes )
u_i4		tag;
u_i4	nbytes;
{
	return FEreqlng( tag, nbytes, FALSE, (STATUS *)NULL );
}

GLOBALDEF CODETABLE Dec_tab [] =
{
	{AOO_i4, 0, NULL, NULL, NULL, analyze_Dtab_vers},
	{AOO_i4, 0, NULL, NULL, NULL, vproc_noop},
	{AOO_array, sizeof(i4), talloc, oo_gdec, derr, NULL},
	{AOO_array, sizeof(IL_DB_VDESC), talloc, ind_dec, derr, NULL},
	{AOO_array, sizeof(IL), talloc, oo_gdec, derr, NULL},
	{AOO_array, sizeof(FDESCV2), sym_alloc, sym_dec, derr, NULL},
	{AOO_array, sizeof(char *), talloc, oo_gdec, derr, NULL},
	{AOO_array, sizeof(char *), talloc, oo_gdec, derr, NULL},
	{AOO_array, sizeof(i4), talloc, oo_gdec, derr, NULL},
	{AOO_array, sizeof(DB_TEXT_STRING *), talloc, hex_dec, derr, NULL},
	{AOO_array, sizeof(char *), talloc, oo_gdec, derr, NULL}
};

GLOBALDEF i4	Dtab_num = sizeof(Dec_tab)/sizeof(CODETABLE);
