/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
+* Filename:    eqcob.h
** Purpose:     Define constants that are known only to the Cobol dependent
-*              parts of the EQUEL and ESQL grammars.
** History:
**	08-dec-1992 (lan)
**		Added a constant for the datahandler.
**      09-sep-1995 (sarjo01)
**		Added #define MF_COBOL if NT
**	24-apr-1996 (muhpa01)
**	    Added support for Digital Cobol (DEC_COBOL).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Allow testing of DG preprocessor to be done on Unix.
*/
# ifdef DG_TEST_ON_UNIX
#     undef UNIX
#     define DGC_AOS
# endif /* DG_TEST_ON_UNIX */
 
/*
** Cobol data types
*/
# define	COB_COMP	0		/* Computational */
# define	COB_1		1		/* Comp-1	 */
# define	COB_2		2
# define	COB_3		3
# define	COB_4		4
# define	COB_5		5
# define	COB_6		6
# define	COB_NOTYPE	7		/* No usage */
# define	COB_DISPLAY	8		/* Usage Display */
# define	COB_RECORD	9		/* Structure type */
# define	COB_INDEX	10		/* Usage Index */
# define	COB_NUMDISP	11		/* Numeric with Usage Display */
# define	COB_EDIT	12		/* Numeric edited */
# define	COB_BADTYPE	13		/* Unsupported type */


# define	COB_MAXCOMP	10	/* Max Comp length we support */
# define        COB_MAXI3        6      /* S9(6) is three bytes on DG */


/*
** gc_dot is defined in cobgen.c and controls the output of dot-newlines
** at the end of statements.
*/
GLOBALREF bool	gc_dot;

/*
** This structure is pointed at by the SYM st_value field, to keep track of 
** extra information that Eqcbl needs to generate variable usage calls. Most
** of the values are set in different routines in cobtypes.c.
**
** Note: This relies on the fact that pointers are compatible.
*/
typedef struct _cob_sym {
    struct _cob_sym 	*cs_next; 	/* Pointer for free list */
    i1			cs_lev;		/* Level number (maybe useless) */
    i1			cs_type;	/* Cobol type (not Equel type) */
    /*
    ** Union for either:
    **  1.  Length and scale of numeric types;
    **  2.  Length of picture strings.
    */
    union {
	struct {
	    i1	cob__len;	/* Cobol length */
	    i1	cob__scale;	/* Picture scale (digits after 'v') */
	} cob__num;		/* Numeric type */
	i2	cob__slen;	/* Length for character strings */
    } cob__union;
} COB_SYM;

# define	cs_nlen		cob__union.cob__num.cob__len
# define	cs_nscale	cob__union.cob__num.cob__scale
# define	cs_slen		cob__union.cob__slen

# define	COB_NOSIGN	0100		/* Unsigned numerics */

# define	COB_MASKTYPE( val )	((val) & ~COB_NOSIGN)

/*
** VMS, CMS and DG support COB_FLOATS.
** If the COBOL on your system does not allow system floats as COMP-1 or
** COMP-2 then you don't need this.
**
** CMS support binary compiled forms so define EQ_ADD_FNAME to allow
** the rules to supress error messages on file names.
**
** UNIX automatically assumes MF COBOL.
*/

# ifdef CMS
#  define	COB_FLOATS
#  define	EQ_ADD_FNAME
# endif /* CMS */

# ifdef VMS
#  define	COB_FLOATS
# endif /* VMS */

# ifdef NT_GENERIC 
#  define	MF_COBOL
# endif /* NT_GENERIC */

# ifdef UNIX
# ifdef axp_osf
#  define DEC_COBOL              
#  define COB_FLOATS             -- supported by DEC Cobol
# else
#  define	MF_COBOL
/* #  define	EQ_ADD_FNAME	-- no longer true for MF COBOL */
# endif /* axp_osf */
# endif /* UNIX */

# ifdef DGC_AOS
#  define	COB_FLOATS
# endif /* DGC_AOS */


/* 
** Routines to process Cobol types and picture strings
*/
void	cob_to_eqtype();
void	cob_use_type();
i4	cob_type();
char	*cob_picture();
i4	cob_prtsym();
i4	cob_clrsym();
COB_SYM	*cob_newsym();
void	cob_svinit();
void	cob_svusage();
void	cob_svoccurs();
void	cob_svpic();
i4	cob_rtusage();
bool	cob_rtoccurs();
char	*cob_rtpic();

# define	COBCLOSURE	0	/* Closure for Cobol */

/*
** Table of II calls -
** Correspond to II constants and the number of arguments.  For COBOL, the
** II constant is used as an index to eventually reach the intended II
** function.  Groupings and special cases are handled in gen__II.
*/

typedef struct {
    char	*g_IIname;		/* Name of function */
    i4		(*g_func)();		/* Argument dumper */
    i4		g_flags;		/* Argument flags */
    i4		g_numargs;		/* How many  */
} GEN_IIFUNC;

/* Constants to be used by argument dumping routines */
# define	G_NULL		0x0000		/* No flags value */
# define	G_RET		0x0001		/* Value returned from beyond */
# define	G_SET		0x0002		/* Set a value */
# define	G_RETSET	0x0004		/* RET, but in SET position */
# define	G_FUNC		0x0008		/* Is a function */
# define	G_LONG		0x0010		/* Needs long string variable */
# define	G_DYNSTR	0x0020		/* Dyn SQL loong string */
# define	G_DESC		0x0040	      	/* Needs to be BY DESCRIPTOR */ 
# define	G_SPEC		0x0080	      	/* Last arg to this call is  */
					      	/* special -- an IILONG      */
# define	G_ISVAR         0x0100        	/* Is a user variable */
# define	G_LONGARG	0x0200	      	/* Generate numerics as LONGS */
# define	G_RTIF		0x0400		/* Is a function iff rt if */
# define	G_DAT		0x0800		/* Datahandler */

# define	SY_NORMAL	0		/* Symtab name space flag */

/* Mechanism to store COBOL arguments by - see gc_args in code generator */
# define	GCmREF		0
# define	GCmVAL		1
# define	GCmDESC		2
# define	GCmSTDESC	3
# define	GCmOMIT		4
# define	GCmFREE		5
# define	GCmDUMP		6

/* Cobol descriptor for strings -- set scale to TRIM */
# define	TRIM		1
# define	NO_TRIM		2

/*
** COBOL margins for ANSI/card formatted code
*/

# define	COB_B	"           "
