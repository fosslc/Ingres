/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<fdesc.h>
#include	<ifid.h>
#include	<ade.h>
#include	<iltypes.h>
#include	<frmalign.h>
#include	<ifrmobj.h>
#include	"iamstd.h"
#include	"iamtbl.h"
#include	"iamfrm.h"

/**
** Name:	iamfdconv.c - convert AOMFRAME to IFRMOBJ
**
** Description:
**
** History:
**	2/5/89 - first abstracted from iamftch.c  (billc)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:	iiamcfConvertFrame - take an AOMFRAME and produce an IFRMOBJ
**
** Description:
**	takes an encoded frame AOMFRAME and creates 
**	a corresponding IFRMOBJ.  (Further processing may be necessary because
**	of some architectural cheating we do.
**
** Inputs:
**	afrm	{AOMFRAME *}  dbms frame to convert
**	tag	{u_i4}  Storage tag for allocation.
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
**	2/5/89 - first abstracted from iamftch.c
**	01/09/91 (emerson)
**		Remove 4K (?) limit on number of floating-point constants
**		in a frame or procedure (allow up to 32K of them).
**	22-jun-92 (davel)
**		added support for decimal constants; move both the string
**		representation of the decimal literal as well as calculate
**		the precision & scale (which is stored with the string in
**		the IFRMOBJ).
*/

STATUS
iiamcfConvertFrame (afrm, tag, rfrm)
AOMFRAME *afrm;
u_i4	tag;
IFRMOBJ **rfrm;
{
	IFRMOBJ *frm;
	i4	i;
	STATUS		rc;
	STATUS  convertFrame();

	frm = (IFRMOBJ *)FEreqmem(tag, (u_i4)sizeof(*frm), FALSE, &rc);
	if (frm == (IFRMOBJ *) NULL)
		return rc;

#ifdef AOMDTRACE
	IIAMdump( NULL, ERx("INTERNAL FRAME STRUCTURE"),
			afrm, sizeof(AOMFRAME)
	);
	aomflush();
#endif

	/*
	** Symbol table.
	*/
	frm->sym.fdsc_zero = 0;
	frm->sym.fdsc_version = FD_VERSION;
	frm->sym.fdsc_cnt = afrm->num_sym + 1;	/* includes NULL element */
	if ( afrm->num_sym == 0 )
	{
		/*
		** if there isn't a symbol table, nothing will have been
		** allocated.  Allocate the NULL array terminator.  Also,
		** note that a FDESCV2 is being allocated and then cast to
		** an OFDESC.  The other fields in the FDESC member 'frm->sym'
		** will correctly type the structure.
		*/
                if ( (frm->sym.fdsc_ofdesc =
			(OFDESC *)FEreqmem(tag, (u_i4)sizeof(FDESCV2), TRUE,
					&rc)) == NULL )
		{
			return rc;
		}
	}
	else
	{
		frm->sym.fdsc_ofdesc = (OFDESC *)afrm->symtab;
	}

	/*
	** floating constant array.  DB contains ASCII strings,
	** we convert here to have both representations around.
	*/
	frm->num_fc = afrm->num_fc;
	if ( afrm->num_fc == 0 )
	{
		frm->f_const = NULL;
	}
        else 
	{
		if ( (frm->f_const = (FLT_CONS *)FEreqlng(tag,
			(u_i4)sizeof(FLT_CONS) * afrm->num_fc, 
			FALSE, &rc)) == NULL )
		{
			return rc;
		}

		for ( i = 0 ; i < frm->num_fc ; ++i )
		{
			(frm->f_const)[i].str = (afrm->f_const)[i];
			CVaf((afrm->f_const)[i], '.', &((frm->f_const)[i].val));
		}
	}

	/*
	** decimal constant array.  DB contains ASCII strings,
	** we also store the internal decimal representation, as well as
	** the precision/scale to be kept with the character string.
	*/
	frm->num_dc = afrm->num_dc;
	if ( afrm->num_dc == 0 )
	{
		frm->d_const = NULL;
	}
        else 
	{
		if ( (frm->d_const = (DEC_CONS *)FEreqlng(tag,
			(u_i4)sizeof(DEC_CONS) * afrm->num_dc, 
			FALSE, &rc)) == NULL )
		{
			return rc;
		}

		for ( i = 0 ; i < frm->num_dc ; ++i )
		{
			char *cp;
			i4 prec, scale, len;

			/* copy the string pointer, and use afe to calculate
			** the length, prec, and scale from the string 
			** representation. Then allocate space for the encoded
			** packed decimal representation, and use CVapk
			** to convert the string to the internal form.
			** Also store the precision/scale in the IFRMOBJ.
			*/
			cp = (frm->d_const)[i].str = (afrm->d_const)[i];
			_VOID_ afe_dec_info(cp, '.', &len, &prec, &scale);
			(frm->d_const)[i].prec_scale =  
					DB_PS_ENCODE_MACRO(prec, scale);
			if ( ((frm->d_const)[i].val = (char *)FEreqlng(tag,
				(u_i4)len, FALSE, &rc)) == NULL )
			{
				return rc;
			}
			_VOID_ CVapk( cp, '.', prec, scale, 
					(PTR)(frm->d_const)[i].val);
		}
	}

	frm->dbd = afrm->indir;
	frm->stacksize = afrm->stacksize;
	frm->num_dbd = afrm->num_indir;

	frm->num_il = afrm->num_il;
	frm->il = afrm->il;

	frm->i_const = afrm->i_const;
	frm->num_ic = afrm->num_ic;

	frm->c_const = afrm->c_const;
	frm->num_cc = afrm->num_cc;

	frm->num_xc = afrm->num_xc;
	frm->x_const = afrm->x_const;

	frm->tag = tag;

	get_align(afrm, &(frm->fa));

#ifdef AOMDTRACE
	ifrmdump(frm);
#endif

	*rfrm = frm;
	return OK;
}

#ifdef AOMDTRACE

/* IFRMOBJ dumper, for tracing only. */
static
ifrmdump(frm)
IFRMOBJ *frm;
{
	IIAMdump( NULL, ERx("EXTERNAL FRAME STRUCTURE"),
			frm, sizeof(IFRMOBJ)
	);
	IIAMdump( NULL, ERx("IL array"),
			frm->il, (i4)frm->num_il * sizeof(i2)
	);
	IIAMdump( NULL, ERx("DB_FDESC"),
			frm->dbd, (i4)frm->num_dbd * sizeof(IL_DB_VDESC)
	);
	IIAMdump( NULL, ERx("INTEGERS"),
			frm->i_const, (i4)frm->num_ic * sizeof(i4)
	);
	IIAMdump( NULL, ERx("FLOATING POINT"),
			frm->f_const, frm->num_fc * sizeof(f8)
	);
	IIAMsymdump( NULL, frm->sym.fdsc_ofdesc, frm->sym.fdsc_cnt );
	IIAMstrdump( NULL, frm->c_const, frm->num_cc );

	aomflush();
}
#endif

