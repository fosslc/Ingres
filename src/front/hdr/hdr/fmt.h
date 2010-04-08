/*
**	Copyright (c) 2004 Ingres Corporation
*/
#ifndef FMT_H_INCLUDED
#define FMT_H_INCLUDED


# include	<erfm.h>

/*
**	Header file for the Output formatting routines.
**
** History
**	9-25-91 (tom) support for input masking, moved some defintions
**				up to here so they are visible to window manager specific
**				routines.
**	21-apr-92 (Kevinm)
**            Changed fmt_init to IIfmt_init since it is a fairly global
**            routine.  The old name, fmt_init, conflicts with a Green Hills
**            Fortran function name
**	02-feb-1993(mohan)
**		Added tags to structures for making automatic prototypes.
**	12-mar-93 (fraser)
**		Changed structure tag name so that it doesn't begin with
**		underscore.
**	09-apr-1996 (chech02)
**	   Windows 3.1 port changes - add more function prototypes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-Nov-2004 (komve01)
**	    Added fmt_cont_line in FMT for copy.in to work correct for 
**      views.Bug 113498.
**	30-Nov-2004 (komve01)
**	   Undo change# 473469 for Bug#113498. 
**	28-Mar-2005 (lakvi01)
**	   Corrected function prototypes.
**	09-Apr-2005 (lakvi01)
**	    Moved some function declarations into NT_GENERIC and reverted back
**	    to old declerations since they were causing compilation problems on
**	    non-windows compilers.
**	13-Sep-2006 (gupsh01)
**	    Added support for Ansi datetime types.
**	24-Aug-2009 (kschendel) 121804
**	    Weary of #include hell, allow multiple includes.
**	    Drop the old WIN16 section, misleading and out of date.  Better
**	    to just reconstruct as needed.
*/

/*
**	Type definition of FMT
*/

/*
**	FMT - structure contains a format specification set up
**		by the fmt_setfmt routine.
*/

typedef struct	 s_FMT
{
	i4	fmt_type;		/* code for type of format spec */
	i4	fmt_width;		/* width, in columns, of output field */
	i4	fmt_prec;		/* # of digits after decimal
					** also used by date templates for time
					** intervals */
	i4	fmt_sign;		/* sign on format for adjusting */
	union
	{
		char	*fmt_template;	/* Contains cobolese template of field
					** This must be set for numeric,
					** date or string templates. */
		char	fmt_char;	/* used by CVfa
					** This must be set for numeric & hex
					** formats. */
		bool	fmt_reverse;	/* used for right-to-left formats
					** This is set for character formats
					** to indicate that the display starts
					** at the right of the display field */
	} fmt_var;
				/* --------the semantics of the items below are
				           defined in fstring.c, and are
					   relevent to string and (some) date
					   templates only */
	char	*fmt_emask;	/* ptr to entry control mask, bit values 
				   defined in fmt/src/format.h (if null, 
				   then input masking not active, or possible)
				   (set by fmt_setfmt) */
	char	*fmt_cmask;	/* ptr to content control mask, either contains
				   constant character (single or double byte),
				   or index into one of 2 class character
				   arrays which control this position of the 
				   template. NOTE: this array is actually
				   2 bytes per position to accomodate double
				   byte characters.
				   (set by fmt_setfmt) */
	char	**fmt_ccary;	/* ptr to array of ptrs to the
				   character class definitions where
				   were defined in this template. */
	i4	fmt_ccsize;	/* total number of elements in
				   FMT.fmt_ccary[] */
				/* --------end of string template items */
}   FMT;

	/* function externs */

#if (NT_GENERIC)
FUNC_EXTERN STATUS	fmt_areasize(PTR, char *, i4 *);
FUNC_EXTERN STATUS	fmt_cvt(PTR, PTR, PTR, PTR, bool, i4);
FUNC_EXTERN STATUS	fmt_deffmt(PTR, PTR, i4, bool, char *);
FUNC_EXTERN STATUS	fmt_size(PTR, PTR, PTR, i4 *, i4 *);
FUNC_EXTERN STATUS	fmt_format(PTR, PTR, PTR, PTR, bool);
FUNC_EXTERN STATUS	fmt_ftot(PTR, PTR, PTR);
FUNC_EXTERN STATUS	fmt_isvalid(PTR, PTR, PTR, bool *);
FUNC_EXTERN STATUS	fmt_workspace(PTR, PTR, PTR, i4 *);
FUNC_EXTERN STATUS	IIfmt_init(PTR, PTR, PTR, PTR);
FUNC_EXTERN STATUS	fmt_multi(PTR, PTR, PTR, PTR, PTR, bool *, bool, ...);
FUNC_EXTERN STATUS	fmt_setfmt(PTR, char *, PTR, PTR **, i4 *);
FUNC_EXTERN STATUS	fmt_kind(PTR, PTR, i4 *);
FUNC_EXTERN STATUS	fmt_fmtstr(PTR, PTR, char *);
FUNC_EXTERN STATUS	fmt_dispinfo(PTR, PTR, char, char *);
FUNC_EXTERN STATUS	fmt_isreversed(PTR, PTR, bool *);
FUNC_EXTERN i4		fmt_stvalid(PTR, register char *, register i4,
				    register i4, bool *);
FUNC_EXTERN i4		fmt_stposition(register PTR, register i4, register i4);
FUNC_EXTERN i4		fmt_ntposition(char *, i4, i4, i4);
FUNC_EXTERN bool	fmt_stmand(PTR, char *, i4);
FUNC_EXTERN bool	fmt_ntmodify(PTR, bool, char, char *, i4 *, char *, bool *);
#else
FUNC_EXTERN	STATUS	fmt_areasize();
FUNC_EXTERN	STATUS	fmt_cvt();
FUNC_EXTERN	STATUS	fmt_deffmt();
FUNC_EXTERN	STATUS	fmt_size();
FUNC_EXTERN	STATUS	fmt_format();
FUNC_EXTERN	STATUS	fmt_ftot();
FUNC_EXTERN	STATUS	fmt_isvalid();
FUNC_EXTERN	STATUS	fmt_workspace();
FUNC_EXTERN	STATUS	IIfmt_init();
FUNC_EXTERN	STATUS	fmt_multi();
FUNC_EXTERN	STATUS	fmt_setfmt();
FUNC_EXTERN	STATUS	fmt_kind();
FUNC_EXTERN	STATUS	fmt_fmtstr();
FUNC_EXTERN	STATUS	fmt_dispinfo();
FUNC_EXTERN	STATUS	fmt_isreversed();
FUNC_EXTERN	i4		fmt_stvalid();
FUNC_EXTERN	i4		fmt_stposition();
FUNC_EXTERN	i4		fmt_ntposition();
FUNC_EXTERN	bool	fmt_stmand();
FUNC_EXTERN	bool	fmt_ntmodify();
#endif /* NT_GENERIC */

/*
** possible return values for fmt_kind
*/

# define	FM_FIXED_LENGTH_FORMAT		0
# define	FM_VARIABLE_LENGTH_FORMAT	1
# define	FM_B_FORMAT			2
# define	FM_Q_FORMAT			3

/*
** maximum length for a format string
*/

# define	MAX_FMTSTR	255

/*
** maximum length for a input masked field
*/

#define		MAX_FMTMASK	100

/*
** The following are used by rangechk.c (in frame) and strutil.c (in fmt)
** for the processing of qbf-type pattern matching characters.
*/

/* The following are inputs to the pattern matching routines. 
   Note: these are input values to the flags parameter of fcvt_sql() */
#define		PM_NO_CHECK	0
#define		PM_CHECK	1
#define		PM_GTW_CHECK	2
#define		PM_CMD_MASK	3	/* mask to isolate the bits above */
#define		INP_MASK_ACTIVE	0x40	/* bit to indicate that input masking
					   is active */

/* The following are ouputs from the pattern matching routines. */
#define		PM_NOT_FOUND	0
#define		PM_FOUND	1
#define		PM_USE_ESC	2
#define		PM_QUEL_STRING	3

/*
** The forms system needs a special marker to indicate that the last
** byte of the formatted buffer is actually the first byte of a 
** 2-byte character.
*/
#define		F_BYTE1_MARKER	DB_PAT_ONE


/*
**	Sign types, values found in FMT.fmt_sign
*/

# define	FS_NONE		0	/* no sign specified */
# define	FS_PLUS		1	/* plus sign for right just */
# define	FS_MINUS	2	/* minus sign for left just */
# define	FS_STAR		3	/* star for center */

/*
**	Format types, values found in FMT.fmt_type
*/

# define	F_ERROR		-1	/* error format code */
# define	F_C		1	/* character format 'c' */
# define	F_F		2	/* floating format 'f' */
# define	F_E		3	/* scientic format 'e' or 'E' */
# define	F_G		4	/* either 'f' or 'e' format */
# define	F_N		5	/* same as 'g', no align */
# define	F_B		6	/* blank out the field */
# define	F_I		7	/* integer format 'i' */
# define	F_NT		10	/* numeric template */
# define	F_DT		11	/* date template */

# define	F_CT		20	/* character template */
# define	F_CF		21	/* folding character type */
# define	F_CFE		22	/* same as 'cf' but preserve spaces */
# define	F_CJ		23	/* justified character type */
# define	F_CJE		24	/* same as 'cj' but preserve spaces */
# define	F_CU		25	/* internal format used by
					 * unloaddb(); like F_CF but
					 * also folds on quoted strings.
					 */
# define	F_T		26	/* text format 't' used by terminal
					** monitor */
# define	F_Q		27	/* character format 'q' used by Report
					** Writer */
# define	F_ST		28	/* string datatype template,
					   which also handles input
					   masking constructs */
# define	F_ADT		30	/* ANSI date template */
# define	F_TMWO		31	/* ANSI time without tz template */
# define	F_TMW		32	/* ANSI time with tz template */
# define	F_TME		33	/* ANSI time with local tz template */
# define	F_TSWO		34	/* ANSI time without tz template */
# define	F_TSW		35	/* ANSI time with tz template */
# define	F_TSTMP		36	/* ANSI time with local tz template */
# define	F_INYM		37	/* ANSI interval day to second */
# define	F_INDS		38	/* ANSI interval day to second */


#endif /* FMT_H_INCLUDED */
