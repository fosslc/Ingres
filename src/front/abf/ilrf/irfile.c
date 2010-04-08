/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
#include	<lo.h>
#include	<nm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<er.h>
#include	"irstd.h"
#include	"irmagic.h"

#define SUFFIX ERx("tmp")
#define PREFIX ERx("ilrf")

/**
** Name:	irfile.c -	Interpreted Frame Object File I/O Module.
**
** Description:
**	ILRF temp file i/o routines.  file_read is included here to
**	allow knowledge of the temp file data to be localized to
**	this source file.  The caller passes a NULL ohdr to indicate
**	read from temp file, otherwise a read from the specified image.
**
** History:
**	Revision 5.1  86/08  bobm
**	Initial revision.
**
**	Revision 6.5
**	22-jun-92 (davel)
**		added debug tracing for decimal literals.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**/

#ifdef ILRFDTRACE
GLOBALREF FILE *Fp_debug;
#endif

GLOBALREF QUEUE F_list;

#ifdef CDOS
static bool Tempswap = FALSE;
#endif

static bool	Tempexists = FALSE;
static OLIMHDR	Temp_ohdr ZERO_FILL;
static char	Tempname[MAX_LOC+1] = {EOS};

/*{
** Name:	IIORulUnLink - unlink temporary
**
** Description:
**	ILRF to unlink temporary file to free up file space.  Any frames
**	cached therein will have to come from the database again.
**
** Inputs:
**	irblk	ilrf client
**
** Return:
**		OK		success
**		ILE_CLIENT	not a legal client
**		ILE_FRDWR	can't unlink file
**
** History:
**	8/86 (rlm)	written
*/

STATUS
IIORulUnLink(irblk)
IRBLK *irblk;
{
	/* see that we have a legal client */
	CHECKMAGIC (irblk,ERx("ul"));

	if (!Tempexists)
		return (OK);

	/* empty the list of file-cached frames */
	list_zero(&F_list);

	/* close and delete file */
	Tempexists = FALSE;
	return(IIOIidImgDelete(&Temp_ohdr));
}

/*
** temp_open creates and opens the temporary file
*/
static STATUS
temp_open()
{
	STATUS		rc;
	LOCATION	tmp;

	if ( (rc = NMloc(TEMP, PATH, (char *)NULL, &tmp)) == OK  &&
			(rc = LOuniq(PREFIX, SUFFIX, &tmp)) == OK )
	{
		char	*fstr;

		LOtos(&tmp, &fstr);
		STcopy(fstr, Tempname);

		rc = IIOIioImgOpen(Tempname, ERx("rw"), &Temp_ohdr);
	}

	Tempexists = (bool)( rc == OK );

# ifdef ILRFDTRACE
	fprintf(Fp_debug,ERx("Temp file open: %d\n"),Tempexists);
# endif
#ifdef CDOS
	Tempswap = FALSE;
#endif
	return rc;
}

STATUS
file_read(ohdr, pos, tag, frm)
OLIMHDR	*ohdr;
i4	pos;
i2	tag;
IFRMOBJ	**frm;
{
	STATUS rc;

# ifdef PCINGRES
	char		*MEerrfunc();
	char		*saveptr;

	/* cannot have error function active here since it might try
	   to write out a frame and mess us up while we are trying
	   to read in this one */
	saveptr = MEerrfunc(NULLPTR);
# endif

	if (ohdr == NULL)
	{

# ifdef ILRFDTRACE
		fprintf(Fp_debug,ERx("Temp file read: %d\n"),Tempexists);
# endif
		if (! Tempexists)
		{
			rc = temp_open();
			if (rc != OK)
				return (rc);
		}
		ohdr = &Temp_ohdr;
	}

	rc = IIOIfrFrmRead(ohdr, pos, tag, frm);

/*
** put debug tracing here rather than in IOI because we don't want
** IOI calling back into the IAOM dump routines.
*/
#	ifdef ILRFDTRACE
	if (rc == OK)
	{
		IIAMdump(Fp_debug,ERx("IIOI RETURNED FRAME STRUCTURE"),
							*frm,sizeof(IFRMOBJ));
		IIAMdump(Fp_debug,ERx("IL array"),(*frm)->il,
					(i4)(*frm)->num_il * sizeof(i2));
		IIAMdump(Fp_debug,ERx("IL_DB_VDESC"),(*frm)->dbd,
			(i4)(*frm)->num_dbd * sizeof(IL_DB_VDESC));
		IIAMdump(Fp_debug,ERx("INTEGERS"),(*frm)->i_const,
					(i4)(*frm)->num_ic * sizeof(i4));
		IIAMdump(Fp_debug,ERx("FLOATING POINT"),(*frm)->f_const,
					(i4)(*frm)->num_fc * sizeof(FLT_CONS));
		IIAMdump(Fp_debug,ERx("HEX STRINGS"),(*frm)->x_const,
				(i4)(*frm)->num_xc * sizeof(DB_TEXT_STRING *));
		IIAMdump(Fp_debug,ERx("PACKED DECIMAL"),(*frm)->d_const,
					(i4)(*frm)->num_dc * sizeof(DEC_CONS));
		IIAMsymdump(Fp_debug,(*frm)->sym.fdsc_ofdesc,
						(*frm)->sym.fdsc_cnt);
		IIAMstrdump(Fp_debug, (*frm)->c_const, (*frm)->num_cc);
	}
#	endif

# ifdef PCINGRES
	/* reset the error routine */
	MEerrfunc(saveptr);
# endif

	return (rc);
}

STATUS
temp_write(frm, pos)
IFRMOBJ	*frm;
i4	*pos;
{
	STATUS rc;

# ifdef ILRFDTRACE
	fprintf(Fp_debug,ERx("Temp file write: %d\n"),Tempexists);
# endif

	if (!Tempexists)
	{
		rc = temp_open();
		if (rc != OK)
			return (rc);
	}
	rc = IIOIfwFrmWrite(&Temp_ohdr, frm, pos);

#ifdef ILRFDTRACE
	fprintf(Fp_debug,ERx("IIOIfw return: %d, pos %d\n"),rc,*pos);
#endif

	return (rc);
}

#ifdef CDOS
temp_swap()
{
	if (Tempexists && !Tempswap)
	{
		IIOIisImgSwap(&Temp_ohdr);
		Tempswap = TRUE;
	}
}

temp_unswap()
{
	if (Tempexists && Tempswap)
	{
		IIOIiuImgUnswap(&Temp_ohdr, "rw");
		Tempswap = FALSE;
	}
}
#endif
