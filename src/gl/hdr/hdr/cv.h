/*
**  Copyright (c) 2004 Ingres Corporation
*/

# ifndef CV_HDR_INCLUDED
# define CV_HDR_INCLUDED 1
#include    <cvcl.h>

/*
**  Name:	CV.h	- Define CV function externs
**
**  Specification:
**
**  Description:
**	Contains CV function externs
**
**  History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	13-jun-1993 (ed)
**	    added packed decimal routines required by code
**	22-Jul-1993 (daveb)
**	    Add CVaptr, CVaxptr, CVptra, CVptrax.
**      31-aug-1994 (mikem)
**          Created CV_ABS_MACRO() to solve bug #64261.
**      31-May-1995 (wooke01)
**          Added extern double fabs(); To stop warning
**          fabs:domain error occurring when ingbuild calls
**          iigenres.  Problem was caused by fabs not being
**          prototyped. Found while porting usl_us5.
**       31-May-1995 (wooke01)
**          Deleted line just added above as need to investigate
**          further that this is a generic change
**	24-jul-1995 (albany)
**	    Updated cvfa parameters.
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of cv.h.
**	18-sep-1998 (thoda04)
**	    Add prototype for CVuahxl.
**      14-feb-00 (loera01) bug 100276
**	    Added prototype for CVrxl.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	03-aug-2001 (somsa01)
**	    Added the prototypes for the 64-bit specific functions.
**      30-jul-2002 (horda03) Bug 106435
**          Added prototype for CVula(). Convert unsigned long to ascii.
**	05-sep-2003 (abbjo03)
**	    Add prototype for CVla8.
**	15-oct-2003 (somsa01)
**	    Corrected prototype for CVal8(), CVahxl8() and CVuahxl8().
**	2-Jan-2004 (schka24)
**	    Try again with CVal8 now that we have a more dependable i8
**	    definition in compat.h.
**	20-Apr-04 (gordy)
**	    Removed definition for defunct CVtonet().
**      10-jun-2004 (loera01)
**          Added CVal8().
**      05-Sep-2004 (zhahu02)
**          Correcting a typo in the previous change .
**	21-Feb-2005 (thaju02)
**	    Added CVula8().
**	29-Nov-2010 (frima01) SIR 124685
**	    Removed CL_PROTOTYPED from some declarations.
**	    Add prototype for CVl8pk, CVinit and CVlx8.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill remaining CL_PROTOTYPED (always on now).
*/


/*
** Define pointer-vector array for CV functions which
** have single byte or multi byte variants
*/
struct  _CV_FUNCTIONS {
    STATUS  (*IICVal)(
	    char    *a, 
	    i4 *i);
    STATUS  (*IICVal8)(
	    char    *a, 
	    i8 *i);
    VOID    (*IICVlower)(
	    char	*string);
    VOID    (*IICVupper)(
	    char	*string);
};

typedef struct  _CV_FUNCTIONS   CV_FUNCTIONS;

# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF       CV_FUNCTIONS    CV_fvp;
#else
GLOBALREF       CV_FUNCTIONS    CV_fvp;
#endif



#define CVal	CV_fvp.IICVal
#define CVal8	CV_fvp.IICVal8
#define CVlower	CV_fvp.IICVlower
#define CVupper	CV_fvp.IICVupper

FUNC_EXTERN STATUS  CVaf(
	    char    *str, 
	    char    decimal, 
	    f8	    *val
);

FUNC_EXTERN STATUS  CVahxl(
	    char    *str,
	    u_i4 *result
);
FUNC_EXTERN STATUS  CVuahxl(
	    char    *str,
	    u_i4    *result
);


FUNC_EXTERN STATUS  CVal_DB(
	    char    *a, 
	    i4 *i
);


FUNC_EXTERN STATUS  CVal_SB(
	    char    *a, 
	    i4 *i
);


FUNC_EXTERN STATUS  CVal8_DB(
	    char    *a, 
	    i8 *i
);


FUNC_EXTERN STATUS  CVal8_SB(
	    char    *a, 
	    i8 *i
);


FUNC_EXTERN STATUS  CVan(
	    char    *string, 
	    i4	    *num
);

FUNC_EXTERN STATUS  CVapk(
	    char    *string,
	    char    decpt, 
	    i4	    prec, 
	    i4	    scale, 
	    PTR	    pk
);

FUNC_EXTERN STATUS  CVfa(
	    f8	    value, 
	    i4	    width,
	    i4	    prec,
	    char    format,
	    char    decchar,
	    char    *ascii,
	    i2	    *res_width
);

FUNC_EXTERN VOID    CVla(
	    i4	i,
	    char	*a
);

FUNC_EXTERN void    CVla8(
	    i8		i,
	    char	*a);

#if !defined(WIN32) && !defined(vms)
typedef unsigned long long u_ll;
FUNC_EXTERN void    CVui8a(
	    u_ll	i,
	    char	*a);
#endif

FUNC_EXTERN VOID    CVula(
	    u_i4	i,
	    char	*a
);

FUNC_EXTERN void    CVula8(
	    u_i8	i,
	    char	*a);


FUNC_EXTERN VOID    CVlower_DB(
	    char	*string
);

FUNC_EXTERN VOID    CVlower_SB(
	    char	*string
);


FUNC_EXTERN VOID    CVlx(
	    u_i4	num, 
	    char	*string
);

FUNC_EXTERN VOID    CVna(
	    i4		num,
	    char	*string
);

FUNC_EXTERN STATUS  CVol(
	    char	octal[], 
	    i4	*result
);

FUNC_EXTERN STATUS  CVpka(
	    PTR		pk,
	    i4		prec,
	    i4		scale,
	    char        decpt,
	    i4          field_wid,
	    i4          frac_digs,
	    i4		options,
	    char	*str,
	    i4		*res_wid
);


FUNC_EXTERN VOID    CVupper_DB(
	    char	*string
);

FUNC_EXTERN VOID    CVupper_SB(
	    char	*string
);


/* Convert decimal to decimal */
FUNC_EXTERN STATUS  CVpkpk(
	    PTR         pk_in,
	    i4          prec_in,
	    i4          scale_in,
	    i4          prec_out,
	    i4          scale_out,
	    PTR         pk_out
);

FUNC_EXTERN STATUS  CVpkl(
	    PTR		pk,
	    i4		prec,
	    i4		scale,
	    i4		*num
);


FUNC_EXTERN STATUS  CVpkl8(
	    PTR		pk,
	    i4		prec,
	    i4		scale,
	    i8		*num
);

FUNC_EXTERN STATUS  CVlpk(
	    i4		num,
	    i4		prec,
	    i4		scale,
	    PTR		pk
);

FUNC_EXTERN STATUS  CVfpk(
	    f8		num,
	    i4		prec,
	    i4		scale,
	    PTR		pk
);

FUNC_EXTERN STATUS  CVapk(
	    char *	string, 
	    char	decpt, 
	    i4		prec, 
	    i4		scale, 
	    PTR		pk
);

FUNC_EXTERN STATUS  CVpkf(
	    PTR		pk,
	    i4		prec,
	    i4		scale,
	    f8		* fnum
);

/*}
** Name: CV_ABS_MACRO() - return absolute value of argument.
**
** Description:
**	Macro to determine absolute value of integer or float variable.
**	New code should use this function rather than previous instances
**	of abs() and/or fabs().  abs() and fabs() can be system dependent
**	functions and thus only should be used in the CL.
**
** History:
**	
**      31-aug-1994 (mikem)
**	    Created.
**	    Initially created to fix bugs caused by use of fabs() in the
**	    mainline code.  On some if not all UNIX's fabs() is an actual
**	    routine call which takes a double as an argument and returns a
**	    double.  Calls to this code did not have proper includes to 
**	    define the return type so the return code was assumed to be int
**	    (basically returning random data) causing bug #64261.
*/
# define        CV_ABS_MACRO(x)                  (((x) < 0) ? -(x) : (x))


/* These are new, and your CL should prototype them,
   so they don't have CL_PROTPTYPED ifdefs  */

FUNC_EXTERN STATUS CVaptr( char *str, PTR *ptrp );

FUNC_EXTERN STATUS CVaxptr( char *str, PTR *ptrp );

FUNC_EXTERN void CVptra( PTR ptr, char *string );

FUNC_EXTERN void CVptrax( PTR ptr, char *string );

FUNC_EXTERN STATUS CVrxl( char *str, i4 *num ); 

FUNC_EXTERN STATUS CVl8pk(i8 num, i4 prec, i4 scale, PTR pk); 

FUNC_EXTERN VOID CVlx8(long num, char *string);

FUNC_EXTERN VOID CVinit(void);

#if defined(axp_osf) || defined(LP64) || defined(axp_lnx)

FUNC_EXTERN STATUS CVahxl8(register char *str, OFFSET_TYPE *result);

FUNC_EXTERN STATUS CVuahxl8(register char *str, SIZE_TYPE *result);

#endif  /* axp_osf || LP64 || axp_lnx */

#if defined(LP64)

FUNC_EXTERN STATUS CVahxi64(register char *str, OFFSET_TYPE *result);

FUNC_EXTERN STATUS CVuahxi64(register char *str, SIZE_TYPE *result);

#endif  /* LP64 */

# endif /* ! CV_HDR_INCLUDED */
