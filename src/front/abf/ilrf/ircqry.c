
/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
#include "irstd.h"
#include "irmagic.h"

/**
** Name:	ircqry.c - query constants in current frame
**
** Description:
**	Routines for runtime interpreter to query constant values.
**
** Defines:
**	IIORcqConstQuery - get a constant
**	IIORcnConstNums - get number of constants
**	IIORhxcHexConst - get a hex constant
**	IIORfcsFltConstStr - get a float constant string
*/

#define CHECKRANGE(i,n) if (i < 0 || i >= n) return (ILE_ARGBAD)

#ifdef ILRFDTRACE
extern FILE *Fp_debug;
#endif

/*{
** Name:	IIORcqConstQuery - get a constant
**
** Description:
**
** Inputs:
**	irblk	ilrf client
**	idx	constant index
**	type	type of constant, DB_FLT_TYPE, _INT_ or _CHR_
**
** Outputs:
**
**	val	returned pointer to constant of appropriate type.
**
**	return:
**		OK		success
**		ILE_CLIENT	not a legal client
**		ILE_ARGBAD	out of range idx, bad type argument
**		ILE_CFRAME	no current frame
**
** Side Effects:
**
** History:
**	8/86 (bobm)	written
**	26-jun-92 (davel)
**		added 5th argument for value info - currently used only
**		for decimal datatype's precision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

STATUS
IIORcqConstQuery(irblk,idx,type,val, valinfo)
IRBLK *irblk;
i4  idx;
i4  type;
PTR *val;
i4  *valinfo;
{
	IFRMOBJ *fobj;

	CHECKMAGIC(irblk,ERx("cq"));
	CHECKCURRENT(irblk);

	fobj = ((IR_F_CTL *)(irblk->curframe))->d.m.frame;

	/* 1 is the first element */
	--idx;

#ifdef ILRFDTRACE
	fprintf(Fp_debug,ERx("type %d - "),type);
#endif

	switch (type)
	{
	case DB_FLT_TYPE:
		CHECKRANGE(idx,fobj->num_fc);
		*val = (PTR) &((fobj->f_const)[idx].val);
#ifdef ILRFDTRACE
		fprintf(Fp_debug,ERx("floating constant: %12.4g\n"),*((f8 *) *val));
#endif
		break;

	case DB_DEC_TYPE:
		CHECKRANGE(idx,fobj->num_dc);
		*val = (PTR) (fobj->d_const)[idx].val;
		if (valinfo != NULL)
			*valinfo = (i4) (fobj->d_const)[idx].prec_scale;
#ifdef ILRFDTRACE
		fprintf(Fp_debug,ERx("decimal constant: %s\n"), 
				(fobj->d_const)[idx].str);
#endif
		break;

	case DB_INT_TYPE:
		CHECKRANGE(idx,fobj->num_ic);
		*val = (PTR) (fobj->i_const + idx);
#ifdef ILRFDTRACE
		fprintf(Fp_debug,ERx("integer constant: %ld\n"),**val);
#endif
		break;

	case DB_TXT_TYPE:
	case DB_VCH_TYPE:
		CHECKRANGE(idx, fobj->num_xc);
		*val = (PTR) (fobj->x_const[idx]);
		break;

	case DB_CHR_TYPE:
	case DB_CHA_TYPE:
		CHECKRANGE(idx,fobj->num_cc);
		*val = (PTR) (fobj->c_const[idx]);
#ifdef ILRFDTRACE
		fprintf(Fp_debug,ERx("string constant: \"%s\"\n"),*val);
#endif
		break;
	default:
#ifdef ILRFDTRACE
		fprintf(Fp_debug,ERx("illegal\n"),type);
#endif
		return (ILE_ARGBAD);
	}

	return (OK);
}

/*{
** Name:	IIORcnConstNums - get number of constants
**
** Description:
**	Returns number of constants in current frame.
**
** Inputs:
**	irblk	ilrf client
**
** Outputs:
**
**	num_int		number of integer
**	num_flt		number of floats
**	num_str		number of strings
**	num_hex		number of hex's
**	num_dec		number of decimals
**
**	return:
**		OK		success
**		ILE_CLIENT	not a legal client
**		ILE_CFRAME	no current frame
**
** History:
**	3/87 (bobm)	written
**
**	22-jun-92 (davel)
**		added argument for number of decimal constants.
**
*/

STATUS
IIORcnConstNums(irblk,num_int,num_flt,num_str,num_hex,num_dec)
IRBLK *irblk;
i4  *num_flt;
i4  *num_int;
i4  *num_str;
i4  *num_hex;
i4  *num_dec;
{
	IFRMOBJ *fobj;

	CHECKMAGIC(irblk,ERx("cn"));
	CHECKCURRENT(irblk);

	fobj = ((IR_F_CTL *)(irblk->curframe))->d.m.frame;

	*num_flt = fobj->num_fc;
	*num_int = fobj->num_ic;
	*num_str = fobj->num_cc;
	*num_hex = fobj->num_xc;
	*num_dec = fobj->num_dc;

	return (OK);
}

/*{
** Name: IIORfcsFltConstStr	-	Return float constant as a string
**
** Description:
**	Returns a floating point constant as a string (as it was originally
**	stored in the object manager catalogs).
**
** Inputs:
**	irblk		The IRBLK for this session.
**	index		Index of the float constant in the array of such
**			constants.
**
** Outputs:
**	value		The value of the float constant, as a string.
**
**	return:
**		OK		success
**		ILE_CLIENT	not a legal client
**		ILE_ARGBAD	out of range index
**		ILE_CFRAME	no current frame
**
** History:
**	6/87 (bobm)	written
*/

STATUS
IIORfcsFltConstStr(irblk, index, value)
IRBLK	*irblk;
i4	index;
char	**value;
{
	IFRMOBJ *fobj;

	CHECKMAGIC(irblk,ERx("fcs"));
	CHECKCURRENT(irblk);

	fobj = ((IR_F_CTL *)(irblk->curframe))->d.m.frame;

	/* 1 is the first element */
	--index;

	CHECKRANGE(index,fobj->num_fc);
	*value = (fobj->f_const)[index].str;

	return (OK);
}

/*{
** Name: IIORhxcHexConst	-	Return hex constant
**
** Description:
**	Returns the length and value of a hex constant.
**
** Inputs:
**	irblk		The IRBLK for this session.
**	index		Index of the hex constant in the array of such
**			constants.
**
** Outputs:
**	len		The length of the hex constant
**	value		The value of the constant.
**
**	return:
**		OK		success
**		ILE_CLIENT	not a legal client
**		ILE_ARGBAD	out of range index
**		ILE_CFRAME	no current frame
**
** History:
**	6/87 (bobm)	written
*/

STATUS
IIORhxcHexConst(irblk, index, len, value)
IRBLK	*irblk;
i4	index;
i4	*len;
char	**value;
{
	IFRMOBJ *fobj;

	CHECKMAGIC(irblk,ERx("cq"));
	CHECKCURRENT(irblk);

	fobj = ((IR_F_CTL *)(irblk->curframe))->d.m.frame;

	/* 1 is the first element */
	--index;

	CHECKRANGE(index,fobj->num_xc);

	*len = (fobj->x_const[index])->db_t_count;
	*value = (char *) (fobj->x_const[index])->db_t_text;

	return (OK);
}

/*{
** Name: IIORdccDecConst	-	Return decimal constant
**
** Description:
**	Returns the precision/scale, string representation, and value of 
**	a decimal constant.
**
** Inputs:
**	irblk		The IRBLK for this session.
**	index		Index of the hex constant in the array of such
**			constants.
**
** Outputs:
**	ps		The precision/scale of the decimal constrant.
**	strval		The string representation of the constant.
**	value		The value of the constant.
**
**	return:
**		OK		success
**		ILE_CLIENT	not a legal client
**		ILE_ARGBAD	out of range index
**		ILE_CFRAME	no current frame
**
** History:
**	6/92 (davel)	written
*/

STATUS
IIORdccDecConst(irblk, index, ps, strval, value)
IRBLK	*irblk;
i4	index;
i2	*ps;
char	**strval;
char	**value;
{
	IFRMOBJ *fobj;

	CHECKMAGIC(irblk,ERx("cq"));
	CHECKCURRENT(irblk);

	fobj = ((IR_F_CTL *)(irblk->curframe))->d.m.frame;

	/* 1 is the first element */
	--index;

	CHECKRANGE(index,fobj->num_dc);

	*ps = (fobj->d_const)[index].prec_scale;
	*strval = (fobj->d_const)[index].str;
	*value = (fobj->d_const)[index].val;

	return (OK);
}
