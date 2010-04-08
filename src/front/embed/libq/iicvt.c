/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	 <compat.h>
# include	 <me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>

/**
+*  Name: iiconvert.c - Execute numeric conversion (that does not raise
**			exceptions).
**
**  Defines:
**	IIconvert - Convert to and from numeric types.
**
-*
**  History:
**	01-jan-1066	- Written (peb)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
+*  Name: IIconvert - Numeric conversion routine.
**
**  Description:
**	Run-time routine to convert numeric values of one type and length, to a
**	(not necessarily) different type and length.
**
**	The source numeric can be i1, i2, i4, f4, or f8.
**
**	The source number will be converted to the type and length specified
**	in the destination.  It also must be one of i1, i2, i4, f4, or f8.
**
**  Inputs:
**	inp   - i4  * - Input data
**	outp  - i4  * - Output data
**	sf    - i4    - Source number format
**	sl    - i4    - Source number length
**	df    - i4    - Destination number format
**	dl    - i4    - Destination number length
**
**  Outputs:
**	outp - Is set to the new value converted.
**	Returns:
**	    i4  - 0 or -1 to flag success or overflow.
**	Errors:
**	    None
-*
**  History:
**	24-jul-1984	- Do floating point internally in f8 to avoid loss of
**			  precision.  Change "number" to be of type "union",
**			  so we don't implicitly assume that the size of our
**			  union is 8 bytes. (Bug 3754). (mrw)
**	18-jul-1984	- Correct a problem in f4->f8 conversion caused by a
**			  Whitesmiths C deficiency. (scl)
**	10-oct-1986	- Modified to reduce copy overhead. (ncg)
*/

i4
IIconvert(inp, outp, sf, sl, df, dl)
i4		*inp;		/* input area */
i4		*outp;		/* output area */
i4		sf;		/* format of the source number */
register i4	sl;		/* length of the source number */
i4		df;		/* format of the dest */
register i4	dl;		/* length of the dest */
{
    /* Any numeric-conversion type union */
    typedef union {
	i1	i1type;
	i2	i2type;		/* Do not convert any i2's to a i4  */
	i4	i4type;		/* Do not convert any i4's to a i4  */
	f4	f4type;
	f8	f8type;
    } II_CONVNUM;

    II_CONVNUM	num; 		/* Temp buffer */
    f8		f8low, f8high;

    if (sf == df && sl == dl)
    {
	MEcopy((char *)inp, sl, (char *)outp);
	return 0;
    }

    MEcopy((char *)inp, sl, (char *)&num); /* copy number into buffer */
    if (sf != df)
    {
	/*
	** If the source and destination formats are different then the source
	** must be converted to i4 if the dest is integer, otherwise to an f8
	** if the dest is float.
	*/

	if (df == DB_FLT_TYPE)	/* {sf == INT} any_integer->f8 */
	{
	    switch (sl)
	    {
	      case sizeof(i1):
		num.f8type = num.i1type;	/* i1 to f8 */
		break;
	      case sizeof(i2):
		num.f8type = num.i2type;	/* i2 to f8 */
		break;
	      case sizeof(i4):
		num.f8type = num.i4type;	/* i4 to f8 */
		break;
	    }
	    sl = sizeof(f8);	/* sf == INT && df == FLOAT && sl == 8 */
	}
	else	/* {sf == FLOAT && df == INT} any_float->i4 */
	{
	    /* Will the float overflow the integer? */
	    f8low = (f8)MINI4;
	    f8high = (f8)MAXI4;

	    if (sl == sizeof(f8))
	    {
		if (num.f8type > f8high || num.f8type < f8low)
		    return (-1);
		num.i4type = num.f8type;
	    }
	    else if (sl == sizeof(f4))
	    {
		if (num.f4type > f8high || num.f4type < f8low)
		    return (-1);
		num.i4type = num.f4type;
	    }
	    else
		return (-1);
	    sl = sizeof(i4);
	}
    }

    /* "num" is now the same type as destination; convert lengths to match */
    if (sl != dl)
    {
	if (df == DB_FLT_TYPE)	/* df is a FLOAT and "num" is an f8 */
	{
	    if (dl == sizeof(f8))	/* f4->f8 */
	    {
		f8	tempf8;

		/*
		** This is required because of a problem in the Whitesmiths C
		** compiler; without an intermediate step the f8 always ends
		** up zero.
		*/
		tempf8 = num.f4type;   		/* f4 to f8 */
		num.f8type = tempf8;	    	/* f8 to f8 */
	    }
	    else
		num.f4type = num.f8type;	/* f8 to f4 with rounding */
	}
	else			/* df is an INT and "num" is an i4 */
	{
	    switch (dl)
	    {
	      case sizeof(i1):
		if (sl == sizeof(i2))		/* i2 to i1 */
		{
		    if (num.i2type > MAXI1 || num.i2type < MINI1)
			return (-1);
		    num.i1type = num.i2type;
		}
		else				/* i4 to i1 */
		{
		    if (num.i4type > MAXI1 || num.i4type < MINI1)
			return (-1);
		    num.i1type  = num.i4type;
		}
		break;
	      case sizeof(i2):
		if (sl == sizeof(i1))		/* i1 to i2 */
		{
		    num.i2type = num.i1type;
		}
		else				/* i4 to i2 */
		{
		    if (num.i4type > MAXI2 || num.i4type < MINI2)
			return (-1);
		    num.i2type = num.i4type;
		}
		break;
	      case sizeof(i4):
		if (sl == sizeof(i1))		/* i1 to i4 */
		    num.i4type = num.i1type;
		else				/* i2 to i4 */
		    num.i4type = num.i2type;
	    }
	}
    }

    /* Conversion is complete copy the result into outp */
    MEcopy((char *)&num, dl, (char *)outp);

    return (0);
}
