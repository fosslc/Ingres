/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	structure for string type arrays.  Two parrallel arays of
**	strings and their associated numeric values.	
*/
typedef struct
{
	char **str;	/* string array */
	i4 *val;	/* associated numeric values */
} DV_SVAL;

typedef struct g_dv_strc
{
	struct g_dv_strc *link;	/* link for pool of data vectors */
	char *name;		/* name of vector */
	i4 len;		/* length of vector */
	i2 tag;			/* storage tag for vector */
	i2 type;		/* type of data in vector */
	i2 scratch;		/* for use by mapping routines */
	union
	{
		DV_SVAL sdat;
		float *fdat;
		i4 *idat;
	} ar;			/* array of data values */
} GR_DVEC;

/*
	types of data:

		DVEC_MASK & type tells you which item of ar applies.
		The encodes should be unique without these bits, just
		for safety.

		All numeric data is floating point.  Dates are 6 digit
		integers.
*/

#define DVEC_MASK 0xff00
#define DVEC_I 0x100		/* integer - idat item */
#define DVEC_S 0x200		/* string - sdat item */
#define DVEC_F 0x300		/* numeric - fdat item */

#define GRDV_MONEY (DVEC_F|1)	/* money type, dollars in fdat array */
#define GRDV_DATE (DVEC_I|2)	/* date type, dates in idat array */
#define GRDV_NUMB (DVEC_F|3)	/* numeric type, numbers in ndat array */
#define GRDV_STRING (DVEC_S|4)	/* string type, sdat is array of strings */
