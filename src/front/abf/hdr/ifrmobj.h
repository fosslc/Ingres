
/*
** Copyright (c) 2004, Ingres Corporation
*/

/**
** Name:	ifrmobj.h - in memory frame data for IOSL
**
**	NOTE:
**		These structures are relocated through the use of DS in the
**		ioi library.  DO NOT make any changes here without updating
**		the DS templates used by IOI (ioifrm.c).
**
** History:
**	8/86 (bobm)	written
**	3/87 (bobm)	added fa item and version.
**	6/87 (bobm)	hex constants & FLT_CONS instead of just f8's.
**	09-jan-91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails changing num_il from a i4  to a i4.
**		Note that this would be a non-compatible change,
**		except that all presents machines we port to have
**      	i4 and i4 the same.
**
**	Revision  6.5
**	11-jun-92 (davel)
**		Add Decimal constants members, and introduce a IFRMOBJ
**		versioning scheme, based on a similar approach developed
**		by Emerson for W4GL.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


#define FRM_VERSION 2

/*
** Structure representing a floating-point constant.
*/
typedef struct _flt_cons
{
	f8 val;		/* value of constant */
	char *str;	/* value as a string */
} FLT_CONS;

/*
** Structure representing a Decimal constant.
*/
typedef struct _dec_cons
{
	char *str;	/* value as a string */
	char *val;	/* internal value as a byte stream */
	i2 prec_scale;  /* precision/scale */
} DEC_CONS;

typedef struct _ifrmobj
{
	u_i4 tag;		/* memory allocation tag */
	FRMALIGN fa;		/* alignment & version info */
	FDESC sym;		/* symbol table - contains OFDESC ptr, size */
	IL_DB_VDESC *dbd;	/* stack indirection section */
	IL *il;			/* intermediate language */
	char **c_const;		/* character constants */
	FLT_CONS *f_const;	/* floating constants */
	i4 *i_const;		/* integer constants */
	DB_TEXT_STRING **x_const;	/* hexadecimal constants */
	i4 stacksize;		/* total stack size */
	i4 num_dbd;		/* number of stack_id elements */
	i4 num_il;		/* number of il elements */
	i4 num_cc;		/* number of c_const elements */
	i4 num_fc;		/* number of f_const elements */
	i4 num_ic;		/* number of i_const elements */
	i4 num_xc;		/* number of x_const elements */
	DEC_CONS *d_const;	/* decimal constants */
	i4 num_dc;		/* number of d_const elements */
} IFRMOBJ;

/*
** IFRMOBJ_V1: IFRMOBJ structure for frames encoded into the data base
** fa.fe_version will be 1.  IFRMOBJ_V1 is identical to the new IFRMOBJ 
** structure, except that it lacks the last 2 members: d_const and num_dc.
**
** Note: When IAOM retrieves a V1 IFRMOBJ structure from the data base,
** it will create a full-sized IFRMOBJ structure with d_const = NULL
** and num_dc = 0 (fa.fe_version will be left set to 1).
*/

typedef struct _ifrmobj_v1
{
	u_i4 tag;		/* memory allocation tag */
	FRMALIGN fa;		/* alignment & version info */
	FDESC sym;		/* symbol table - contains OFDESC ptr, size */
	IL_DB_VDESC *dbd;	/* stack indirection section */
	IL *il;			/* intermediate language */
	char **c_const;		/* character constants */
	FLT_CONS *f_const;	/* floating constants */
	i4 *i_const;		/* integer constants */
	DB_TEXT_STRING **x_const;	/* hexadecimal constants */
	i4 stacksize;		/* total stack size */
	i4 num_dbd;		/* number of stack_id elements */
	i4 num_il;		/* number of il elements */
	i4 num_cc;		/* number of c_const elements */
	i4 num_fc;		/* number of f_const elements */
	i4 num_ic;		/* number of i_const elements */
	i4 num_xc;		/* number of x_const elements */
} IFRMOBJ_V1;
