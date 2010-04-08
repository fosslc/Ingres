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
** Name:	iametbl.c - drive table
**
** Description:
**	Drive table for encoding/decoding of AOMFRAME structure.
**
**	CAUTION:  This is a template which must exactly match the layout
**		of the AOMFRAME definition.
**
**	This structure defines both decoding / encoding of structure.  To
**	avoid unneeded procedure linking, 2 copies are being maintained.
**	This copy is used for the encoding operation.  The decoding procedures
**	are fatal. Structural changes will have to be reflected in the
**	decoding table also!
**
** History:
**	Revision 6.0  86/08  bobm
**	Initial revision.
**
**	Revision 6.1  88/08  wong
**	Modified to use 'FEreqmem()'.
**
**	Revision 6.5
**	19-jun-92 (davel)
**		added entry for decimal literals array.  Also change es_wr
**		call to pass prefix ans suffix strings instead of char's.
**		Also added static pd_wr() in this module to encode decimal
**		literals. The format for decimal literals will be exactly
**		the same as for floats; i.e. they will just be represented
**		as an array of strings.
**		Also add intializers for new "vproc" member of CODETABLE.
**	22-jul-92 (blaise)
**		Changed vproc_err() from static VOID to VOID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** write procedures - es_wr not pointed to directly.  It is used to
** "collapse" the constant table writers - see below.
*/
STATUS	ind_wr();		/* stack indirection section */
STATUS	es_wr();		/* "generic" string writer */
STATUS	sym_wr();		/* symbol table */
STATUS	il_wr();		/* IL */
STATUS	alg_wr();		/* alignment section */
STATUS	hex_wr();		/* write hexadecimal strings */

/*
** constant table writers - constants are actually passed as already
** encoded strings.  Pass on appropriately to "generic" string writer.
**
** because we want to preserve the users original input for the code
** generator in the case of floating point constants, we save them as
** strings, using the same routine as for character constants.
*/
static STATUS
cc_wr( ptr, num )
char	**ptr;
i4	num;
{
	return es_wr(ptr, num, ERx("\""), ERx("\""), TRUE);
}

static STATUS
ic_wr( ptr, num )
char	**ptr;
i4	num;
{
	return es_wr(ptr, num, ERx("I"), ERx(""), FALSE);
}

VOID
vproc_err( vers, tbladdr, tblsize )
i4		vers;
CODETABLE	**tbladdr;
i4		*tblsize;
{
	IIUGerr(S_AM0013_bad_decode_call, UG_ERR_FATAL, 0);
}

static i4
eerr()
{
	IIUGerr(S_AM0013_bad_decode_call, UG_ERR_FATAL, 0);
}
static PTR
merr()
{
	IIUGerr(S_AM0013_bad_decode_call, UG_ERR_FATAL, 0);
}

GLOBALDEF CODETABLE Enc_tab [] =
{
	{AOO_i4, 0, NULL, NULL, NULL, vproc_err},
	{AOO_i4, 0, NULL, NULL, NULL, vproc_err},
	{AOO_array, sizeof(i4), merr, eerr, alg_wr, NULL},
	{AOO_array, sizeof(IL_DB_VDESC), merr, eerr, ind_wr, NULL},
	{AOO_array, sizeof(IL), merr, eerr, il_wr, NULL},
	{AOO_array, sizeof(FDESCV2), merr, eerr, sym_wr, NULL},	/* symbols */
	{AOO_array, sizeof(char *), merr, eerr, cc_wr, NULL},	/* chars */
	{AOO_array, sizeof(char *), merr, eerr, cc_wr, NULL},	/* floats */
	{AOO_array, sizeof(i4), merr, eerr, ic_wr, NULL},	/* ints */
	{AOO_array, sizeof(DB_TEXT_STRING *), merr, eerr, hex_wr, NULL}, 
	{AOO_array, sizeof(char *), merr, eerr, cc_wr, NULL},	/* decimals */
};

GLOBALDEF i4	Etab_num = sizeof(Enc_tab)/sizeof(CODETABLE);
