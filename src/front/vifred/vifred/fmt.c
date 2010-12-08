/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** fmt.c
**
** routines which deal with fmt structures
**
** Copyright (c) 1985, 2001 Ingres Corporation
**
** History:
**	1/2/85	drh  -- Changed to FEcalloc and FEsalloc in fmtAlloc routine.
**	1/28/85 (peterk) - split vfgetSize() off into vfgetsize.c
**	30-sep-86 (bruceb) Fix for bugs 8683 and 10456.
**		Place further restrictions of allowable display formats.
**	18-nov-86 (bruceb)
**		Re-allow i1 data fmt; disallowed by fix to 10456.
**	16-mar-87 (bruceb)
**		added code to allow reversed formats used in the Hebrew
**		project.  zapped the fmtTyChar routine (not used).
**	03/21/87 (dkh) - Added support for ADTs.
**	11/18/86 (KY)  -- changed STindex,STrindex for Double Bytes characters.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	10/07/87 (dkh) - Fixed jup bug 1078.
**	10/25/87 (dkh) - Error message cleanup.
**	18-jul-88 (marian)
**		Added check after call to fmt_setfmt() to see if a bad format
**		was specified.  The check is done in vifred instead of fmt 
**		because of the way report writer passes formats down to fmt. 
**		Report writer was examined to determine if it could send the
**		correct format down to fmt, but it would mean having to parse
**		the exact string that fmt would be parsing before calling fmt
**		and then calling fmt again.  Adding the check here seemed to
**		be a better solution.
**	01-jun-89 (bruceb)
**		Removed MEfree(blk) from vfchkFmt.  Can't MEfree memory
**		allocated using FEreqmem.
**	06-sep-89 (bruceb)
**		Disallow display formats that are too big.  Arbitrarily
**		setting 'too big' at DB_GW4_MAXSTRING.  This avoids AVs
**		on trying to allocate memory for a form with a huge field.
**	11-oct-89 (sylviap)
**		Made the Q format, FM_Q_FORMAT, illegal for vifred and rbf.
**	18-feb-92 (mgw) Bug #39406
**		Pass DB_GW4_MAXSTRING to IIUGerr() in a variable in vfchkSize()
**		to get proper error msg.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-mar-2001 (somsa01)
**	    Changed maxcharwidth from 10000 to FE_FMTMAXCHARWIDTH.
**	30-nov-2010 (gupsh01) SIR 124685
**	    Prototype cleanup.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<ug.h>
# include	<si.h>
# include	<st.h>
# include	<er.h>
# include	"ervf.h"

FUNC_EXTERN	ADF_CB		*FEadfcb();
FUNC_EXTERN	STATUS		fmt_areasize();
FUNC_EXTERN	STATUS		fmt_setfmt();
FUNC_EXTERN	STATUS		fmt_kind();
FUNC_EXTERN  DB_STATUS adi_tycoerce(ADF_CB	*adf_scb,
				    DB_DT_ID	 adi_did,
				 ADI_DT_BITMASK *adi_dmsk);


bool	vfchkSize();
char	FDtypeChar();
char	*FEsalloc();

/*
** check the format for legality
*/
FMT *
vfchkFmt(str)
char	*str;
{
	FMT	*f = NULL;
	ADF_CB	*ladfcb;
	PTR	blk;
	i4	fmtsize;
	i4	fmtlen;

	if (*str == '\0')
	{
		return(f);
	}

	ladfcb = FEadfcb();
	/*
	**  Bug fix to get eliminate possible
	**  garbage for our fmt string. (dkh)
	*/

	if (fmt_areasize(ladfcb, str, &fmtsize) != OK)
	{
		return(f);
	}
	if ((blk = FEreqmem((u_i4)0, (u_i4)fmtsize, TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("vfchkFmt"));
		return(f);
	}
	if (fmt_setfmt(ladfcb, str, blk, &f, &fmtlen) != OK)
	{
		/*
		** An error used to be issued here.. but all callers
		** will issue errors anyway, so we avoid double error.
		*/

		/*
		**  Return NULL instead of f since f may have
		**  been changed to be not NULL by fmt_setfmt().
		*/
		return(NULL);
	}
	else
	{
		/* 
		** Check to see that the length of str is the same as
		** fmtlen.  If it isn't, there is an invalid format string.
		*/
		if (STlength(str) != fmtlen)
		{
			IIUGerr(E_VF006B_Invalid_format_specif, UG_ERR_ERROR,0);
			/*
			**  Return NULL instead of f since f may have
			**  been changed to be not NULL by fmt_setfmt().
			*/
			return(NULL);
		}
	}
	if (!vfchkSize(f))
	{
		f = NULL;
	}

	return(f);
}

/*
**  Check and only allow fixed length formats for VIFRED and
**  fixed and variable length formats for RBF.
*/
bool
vfchkSize(fmt)
FMT	*fmt;
{
	i4	fmtkind;
	i4	y;
	i4	x;

	if (fmt_kind(FEadfcb(), fmt, &fmtkind) != OK)
	{
		/*
		**  No error message since this should never occur.
		*/
		return(FALSE);
	}
	if (fmtkind == FM_B_FORMAT || fmtkind == FM_Q_FORMAT ||
		(!RBF && fmtkind == FM_VARIABLE_LENGTH_FORMAT))
	{
		return(FALSE);
	}

	/*
	**  If RBF, can only allow c0.w formats and not just a c0 format.
	*/
	if (fmtkind == FM_VARIABLE_LENGTH_FORMAT)
	{
		/*
		**  No error message since this should work.
		*/
		(VOID) fmt_size(FEadfcb(), fmt, NULL, &y, &x);
		if (x == 0)
		{
			return(FALSE);
		}
	}

	/*
	** Allow maximum of DB_GW4_MAXSTRING characters in a display format.
	*/
	if (fmt->fmt_width > DB_GW4_MAXSTRING)
	{
	    y = DB_GW4_MAXSTRING;	/* re-use variable y for this error */
	    IIUGerr(E_VF0142_Format_too_big, UG_ERR_ERROR, 1, &y);
	    return(FALSE);
	}

	return(TRUE);
}

/*
** Save a fmt string for a field.
** Only put a leading "-" if the datatype if not in RBF,
** datatype is coercable to float and there is no current
** floating leading sign indicator.
*/

char *
vfsaveFmt(str)
char	*str;
{
	return(FEsalloc(str));
}


/*
**  User must pass "buf" to avoid referencing memory on the stack
**  that may no longer be valid once we exit the routine. (gb)
*/

VOID
vfFDFmtstr(type, buf)
FLDTYPE *type;
char	*buf;
{
	DB_DATA_VALUE	dbv;

	dbv.db_datatype = type->ftdatatype;
	dbv.db_length = type->ftlength;
	dbv.db_prec = type->ftprec;


	/*
	**  Pass a huge number for maxcharwidth to avoid wrap
	**  format.  This is done so that default formats are
	**  compatible.
	*/
	if (fmt_deffmt( FEadfcb(), &dbv, FE_FMTMAXCHARWIDTH, (bool) FALSE,
			buf ) != OK)
	{
		buf[0] = '\0';
		IIUGerr(E_VF006C_Could_not_create_defa, UG_ERR_ERROR, 0);
	}
}
