
/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    ug.h -	General Utilities Module Library Interface Definitions.
**
** Description:
**	This file contains all of the typedefs and defines necessary
**	to use any of the UG routines.   UG is the general frontend
**	utility library.
**
** History:
**	09-oct-1987 (peter)
**		Written.
**	09-oct-1987 (peter)
**		Add more entries for declarations.
**	09/12/92 (dkh) - Added declaration of IIUGfnFeName().
**	1-oct-1992 (rdrane)
**		Add declaration of IIUGIsSQLKeyWord().  Add protection against
**		multiple includes.
**	12-nov-1992 (sandyd/rdrane)
**		Added include of <er.h> to ensure definition of ER_MSGID.
**	02-feb-92 (essi)
**		Added definitions for the new file check utility.
**	23-feb-1993 (mohan) added tags to structures for proper  prtotyping.
**	12-mar-93 (fraser)
**		Changed structure tag names so that they don't begin with
**		underscore.
**	4-oct-1993 (rdrane)
**		Add prototype for IIUGpx_parmxtract(), and its associated
**		typedefs and defines.
**	27-dec-1994 (canor01)
**		Add prototype for UGcat_now()
**	31-mar-95 (brogr02)
**		Added func prototype for iiuglcd_check()
**	15-mar-96 (thoda04)
**		Added func prototype for IIUGinit()
**	27-feb-96 (lawst01)
**	   Windows 3.1 port changes - add some fcn prototypes.
**	17-may-1996 (chech02)
**	   Windows 3.1 port changes - eliminate compiler warning messages.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-Apr-2005 (lakvi01)
**		Corrected function prototypes.
**      09-Apr-2005 (lakvi01)
**	    Moved some function declarations into NT_GENERIC and reverted back
**	    to old declerations since they were causing compilation problems on
**	    non-windows compilers.
**	26-Aug-2009 (kschendel) b121804
**	    New style prototype for IIUGyn.
**	    Drop the old WIN16 section, misleading and out of date.  Better
**	    to just reconstruct as needed.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added prototype for IIUGhfHtabFind to eliminate gcc 4.3 warnings.
**      15-Jan-2010 (maspa05) bug 123141
**          Added prototypes for FEapply_date_format, FEapply_money_format,
**          FEapply_money_prec, FEapply_decimal and FEapply_null
**	24-Feb-2010 (frima01) Bug 122490
**	    Add function prototypes as neccessary
*	    to eliminate gcc 4.3 warnings.
**/

#ifndef UG_HDR_INCLUDED
#define	UG_HDR_INCLUDED

#include	<erug.h>
#include	<er.h>
#include	<adf.h>

/*
**	Severity Levels for the IIUGerr routine.
*/

# define	UG_ERR_ERROR	0
# define	UG_ERR_FATAL	3

/*
** IIUGpx_parmxtract() typedefs and defines.
*/

typedef struct _ug_px_val
{
	char			*px_value;
	struct _ug_px_val	*px_v_next;
}       UG_PX_VAL;

typedef struct
{
	i4		px_type;
# define	UG_PX_POS		0
# define	UG_PX_FLAG		10
# define	UG_PX_MFLAG		20
# define	UG_PX_EMFLAG		30
	i4	px_e_code;
# define	UG_PX_NO_ERROR		0
# define	UG_PX_ALLOC_ERROR	10	/*
						** Error allocating string
						** buffer or next UG_PX_VAL
						*/
# define	UG_PX_UNBAL_Q		20	/*
						** Encountered EOS while
						** processing a quoted string
						** value.
						*/
# define	UG_PX_NO_VLIST		30	/*
						** Encountered EOS after seeing
						** an '=' (equals).
						*/
# define	UG_PX_SHORT_VLIST	32	/*
						** Encountered EOS after seeing 
						** a ',' (comma).
						*/
# define	UG_PX_MPARMS		40	/*
						** Encountered extraneous text
						** (not '=') after seeing a
						** flag.
						*/
# define	UG_PX_INVALID_FLAG	50	/*
						** Encountered an invalid flag
						** (not CMnmchar() after seeing
						** "-" or "+" introducer).
						*/
# define	UG_PX_INVALID_MFLAG	52	/*
						** Encountered an invalid meta
						** flag (not CMnmchar() and not
						** "-" after  seeing "-/"
						** introducer).
						*/
# define	UG_PX_INVALID_EMFLAG	54	/*
						** Encountered an invalid
						** escaped meta flag (not
						** "/" after  seeing "-/-"
						** introducer).
						*/
	char		*px_name;
	UG_PX_VAL	*px_v_head;
}       UG_PX_IDENT;

/*
**	Declarations of returns
*/

#ifdef NT_GENERIC
VOID	IIUGber(char *, i4, u_i4, i4, i4, ...);
VOID    IIUGeppErrorPrettyPrint(PTR, char *, bool);
VOID    IIUGmsg(char *, bool, i4, ...);
char    *IIUGfemFormatErrorMessage(char *, i4, char *, bool);
STATUS  IIUGuapParse(i4, char **, char *, PTR);
VOID    IIUGerr(u_i4, ...);
char    *IIUGfmt(char *, register i4, char *, i4, ...);
VOID    IIUGfer(PTR, u_i4, i4, i4, ...);
#else
VOID	IIUGber();
VOID	IIUGeppErrorPrettyPrint();
VOID	IIUGmsg();
char	*IIUGfemFormatErrorMessage();
STATUS	IIUGuapParse();
STATUS	FEspawn();
VOID    IIUGerr();
char    *IIUGfmt();
VOID    IIUGfer();
VOID    IIUGlbo_lowerBEobject();
VOID    set_IituMainform();
#endif /* NT_GENERIC */

VOID	IIUGbmaBadMemoryAllocation();
VOID	iiugfrs_setting();
STATUS	IIUGinit(void);
bool	IIUGIsSQLKeyWord();
i4	iiuglcd_langcode();
VOID	IIUGfnFeName();
bool	IIUGyn(char *, STATUS *);
char	*UGcat_now();

/* Function prototypes */

STATUS  iiuglcd_check(VOID);
STATUS  IIUGhfHtabFind();
void    FEapply_date_format(ADF_CB *,char *);
void    FEapply_money_format(ADF_CB *,char *);
void    FEapply_money_prec(ADF_CB *,char *);
void    FEapply_decimal(ADF_CB *,char *);
void    FEapply_null(ADF_CB *,char *);

VOID	IIUGsber(
    DB_SQLSTATE     *db_sqlst,
    char            *msg_buf,
    i4              msg_len,
    ER_MSGID        msgid,
    i4              severity,
    i4              parcount,
    PTR             a1,
    PTR             a2,
    PTR             a3,
    PTR             a4,
    PTR             a5,
    PTR             a6,
    PTR             a7,
    PTR             a8,
    PTR             a9,
    PTR             a10
);

bool	IIUGpx_parmxtract(
    char	    *buf,
    UG_PX_IDENT	    *ugpx,
    TAGID	     ugtag
);


/*
** Name:    feuta.h -	Front-End Argument Parsing Module Definitions.
**
** Description:
**	Defines interface to Argument Parsing Module.
*/

/* Argument Error Number Definitions */

#define NOARGPROMPT	E_UG0010_NoPrompt	/* No user response for prompted
							argument */
#define BADSYNTAX	E_UG0011_ArgSyntax	/* Invalid command syntax */
#define NOARGVAL	E_UG0012_NoArgValue	/* Argument expects value */
#define TOOMANYARGS	E_UG0013_TooManyArgs	/* Too many instances of an
							argument */
#define BADARG		E_UG0014_BadArgument	/* Unknown argument */
#define NOARGUMENT	E_UG0015_ArgMissing	/* Argument required */
#define ARGCONFLICT	E_UG0016_ArgConflict	/* Argument conflicts with
							others */
#define ARGLISTCONF	E_UG0017_ListConflict	/* Argument conflicts with other
							list */
#define ARGOVERRIDE	E_UG0018_Override	/* Argument overrides other(s)*/
#define ARGINTREQ	E_UG0019_ArgIntReq	/* Integer required for
							argument */
#define ARGFLTREQ	E_UG001A_ArgFltReq	/* Floating point number
							required for argument */

/*
** Failure Mode for Returning Arguments.
*/
# define FARG_FAIL	1
# define FARG_ABORT	2
# define FARG_PROMPT	3
# define FARG_OPROMPT	4

/*
** Position of Prompt Arguments
*/

# define PRMPT_ARG	1000

/*}
** Name:    ARGRET -	Argument Parsing Returned Argument Structure.
**
** Description:
**	Argument structure for returned argument from command line.
** History:
**	20-jul-1988 - Added # defines for argument types. (ncg)
*/
typedef struct s_ARGRET
{
    i2	 type;	/* 0 = no data */

# define ARGRET_NONE	0
# define ARGRET_STRING	1
# define ARGRET_INT	2
# define ARGRET_FLOAT	3
# define ARGRET_BOOL	4

    union
    {
	char	*name;	/* type = 1 */
	i4	num;	/* type = 2 */
	f8	val;	/* type = 3 */
	bool	flag;	/* type = 4 */
    } dat;
} ARGRET;

/*}
** Name:   ARG_DESC -	Argument Descriptor.
**
** Description:
**	Argument descriptor array element.  Programs can pass an argument
**	descriptor array to 'IIUGuapParse()' to get all its command-line
**	parameters.
*/
typedef struct s_ARG_DESC
{
    char	*_name;		/* argument name */
    DB_DT_ID	_type;		/* argument type */
    i1		_fail_mode;	/* failure mode */
    PTR		_ref;		/* data reference */
} ARG_DESC;

#endif

/* Symbols for IIUGvfVerifyFile */

#define		UG_IsExistingDir		1
#define		UG_IsWrtExistingDir		2
#define		UG_IsNonExistingFile		3
#define		UG_IsExistingFile		4
#define		UG_IsWrtExistingFile		5
