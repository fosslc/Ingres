/*
** Copyright (c) 1985, 2008 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<ex.h>
# include	<st.h>
# include	<cm.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<afe.h>
# include	<frame.h>
# include	<qg.h>
# include	<multi.h>
# include	<frserrno.h>
# include	<ug.h>
# include	<ui.h>
# include	"erfd.h"

/**
** Name:	rangechk.c	-	QBF range query support
**
** Description:
**	Contains the routine used to check the validity of range specifications
**	as part of the qualification for a query.  Used by QBF and the ABF RTS
**	(for qualification predicates in queries.)  Defines:
**
**	FDrngchk()
**
** History:
**	Created - 08/23/85 (dkh)
**	25-mar-1987	Modified for ADT's and NULLs (drh).
**	04/09/87 (dkh) - Added forward declarations of static routines.
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
**	4-may-1987 (Joe)
**		Changing to use new QG interface for sending several
**		QRYSPECs for a query.
**	19-jun-87 (bruceb) Code cleanup.
**	07/10/87 (dkh) - Fixed jup bug 482.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	07/22/87 (dkh) - Fixed jup bug 435.
**	07/25/87 (dkh) - Fixed jup bug 522.
**	08/14/87 (dkh) - ER changes.
**	09/11/87 (dkh) - Fixed problem with query operators.
**	11/19/87 (kenl)
**		Changed fmt_cvt call to fcvt_sql.  The new routine is the
**		same as the old one with the addition of a new parameter,
**		sqlpat, which gets set to TRUE when a QUEL pattern matching
**		character is changed to its SQL equivalent.
**	11/25/87 (dkh) - Fixed jup bug 1412.
**	12/07/87 (dkh) - Fixed jup bug 1480.
**	12/07/87 (dkh) - Fixed jup bug 1496.
**	12/10/87 (dkh) - Fixed jup bug 1478 & 1489.
**	01/21/88 (dkh) - Fixed SUN compilation problems.
**	15-apr-1988 (danielt)
**		added call to EXdelete() to match EXdeclare()
**	05/20/88 (dkh) - More ER changes.
**	06/18/88 (dkh) - Integrated CMS changes.
**	15-aug-88 (kenl)
**		Added support of ESCAPE clause for INGRES.
**	08/24/88 (dkh) - Changed declaration of static READONLY to
**			 just static to regain shareability of shared
**			 libs on VMS.
**	28-sep-88 (kenl)
**		Before calling fcvt_sql we now convert the query language
**		that is passed to that routine to either DB_QUEL or DB_SQL.
**	29-sep-88 (kenl)
**		Modified call to fcvt_sql().  It now gets passed an
**		indication as to whether to perfrom pattern matching or not
**		and returns in another variable the result of the pattern
**		matching.  Possible results are: PM_NOT_FOUND, PM_FOUND,
**		PM_USE_ESC, PM_QUEL_STRING.
**	09/30/88 (dkh) - Changed to pass large buffer space for character
**			 datatype fields.
**	13-oct-88 (sylviap)
**		Added TITAN changes.  DB_MAXSTRING -> DB_GW4_MAXSTRING.
**	03/22/89 (dkh) - Put in change to reduce size of varchar and
**			 text DBVs passed to backend.
**	06/27/89 (dkh) - Fixed bug 6789.
**	08/03/89 (dkh) - Updated with interface change to adc_lenchk().
**	28-nov-89 (bruceb)	Fix for bug 2624.
**		Turn off 'reverse' fmt status before call to fcvt_sql().
**		This is currently done by (Kludge) modifying the internal
**		fmt structure.  This should be fixed when time permits.
**	26-feb-90 (bruceb)	Fix for bug 9997.
**		makedbv() now returns 'retstat' if !OK, rather than returning
**		nullhdlr(dbvptr).
**	28-jun-90 (kevinm) Bug 21320
**		'FDrngchk()' and 'gettok()' were changed to not skip leading
**		white space if 'oneonly' is set.
**	03-jul-90 (bruceb)	Fix for 31018.
**		Field data error messages now give field title and name when
**		available (instead of 'current field').  (Actual change is
**		buried inside of FDadfxhdr.)
**	1/91 (Mike S) Add IIFDrcsRangeCheckSuffix
**	07/19/93 (dkh) - Changed call to adc_lenchk() to match
**			 interface change.  The second parameter is
**			 now a i4  instead of a bool.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	23-apr-96 (chech02)
**         added function prototypes for windows 3.1 port.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/

/*
**  The states for the parser
*/

# define NDOPER		1	/* looking for an operator */
# define NDOPRND	2	/* looking for an operand */
# define NDBOOL		3	/* looking for a boolean (AND or OR) */
# define NDRPAREN	4	/* looking for a right parenthesis */

/*
**  The token types
*/

#define	BADTOKEN	-1
#define	NOTOK		0	/* no more tokens - end of input */
#define	LPAREN		1	/* left parenthesis */
#define	RPAREN		2	/* right parenthesis */
#define	OPER		3	/* operator */
#define	ANDOR		4	/* boolean - AND or OR */
#define	OPERAND		5	/* operand */

/*
**  The operand types
*/

# define	OP_NODELIM	1	/* operand without quote delimiter */
# define	OP_DQUOT	2	/* double-quote delimitted operand */

# define	MAXST	256	/* max no. chars in qry text string. No single
				   token should exceed this no. */

EX	FDadfxhdlr();
STATUS	IIftrim();
FUNC_EXTERN	STATUS	fmt_isreversed();

GLOBALREF	char	IIFDftFldTitle[];
GLOBALREF	char	IIFDfinFldInternalName[];

static STATUS	rngparse();
static i4	gettok();
static bool	isoper();
static bool	isandor();
static STATUS	getoprnd();
static VOID	skip_white();
static STATUS	addtostr();
static STATUS	putstr();
static STATUS	makedbv();
static STATUS	nullhdlr();
static bool	frsubstr();

/* Operator Name Constants */

static const
	char	_NotEqual[]		= ERx("!="),
			_UnEqual[]		= ERx("<>"),
			_Equal[]		= ERx("="),
			_Less[]			= ERx("<"),
			_LessEqual[]	= ERx("<="),
			_Greater[]		= ERx(">"),
			_GreatrEqual[]	= ERx(">="),
			_Like[]			= ERx("like"),
			_NotLike[]		= ERx("not like"),
			_IsNull[]		= ERx(" is null "),
			_IsNotNull[]	= ERx(" is not null "),
			_And[]			= ERx("and"),
			_Or[]			= ERx("or"),
			_Escape[]		= ERx(" ESCAPE '\\'");

static STATUS		(*qgfunc)() = NULL;	/* Func for each qry-spec */
static PTR		qgparam	= NULL;		/* parameter for qgfunc */
static bool		FDbuild	= FALSE; /* TRUE if building qry-specs */
static bool		FDgotoprnd = FALSE;
static char		*FDsource = NULL;	/* string to parse (from dbv) */
static char		*FDrcksel = NULL;	/* string - tablename.colname */
static i4		FDrslen = 0;		/* nchars in FDrcksel */
static i4		FDq_lang = UI_DML_NOLEVEL;	/* the query language */
static i4		FDnsleft = 0;		/* nchars left in FDsource */
static u_i4		memTag = 0;		/* tag for local allocations */
static ADF_CB		*adfCb = NULL;
static char		stbuf[ MAXST ] ZERO_FILL;	/* string buffer */
static char		*stnext = NULL;		/* next empty space in stbuf */
static i4		stnleft = 0;		/* no. chars left in stbuf */
static char		*loper = NULL;		/* last operator found */
static i4		pm_cmd = PM_NO_CHECK;	/* initially don't look for
						** pattern matching chars. */
static i4		sqlpat = PM_NOT_FOUND;  /* initially assume no pattern
						** matching chars found */
/*{
** Name:	FDrngchk	-	Check a range-type qualification
**
** Description:
**	Check a range-type qualification in a field's display buffer, and
**	optionally, build a query description for the query generator.
**
**	Perform a recursive decent parse on the qualification,
**	and build a query description from it's components.
**
** Inputs:
**	frm		Pointer to the frame containing the field to use
**	fname		Name of the field
**	tfcolname	Name of the tf column, if field is a tablefield
**	source		Ptr to the DB_DATA_VALUE for the field's display
**			buffer.	 MUST be a DB_LTXT_TYPE.  This is the
**			qualification that will be parsed.
**	build		Flag - if TRUE, a query description for the query
**			generator will be built. If FALSE, simple validate
**			that the syntax and data values are 'legal' for the
**			field type.
**	tblname		Table name to use in the qualification
**			If this is NULL, no table name is used in
**			the qualifcation.  If it is not NULL, the
**			table name followed by a '.' is put into
**			the qualifcation.
**
**	colname		Column name to use in the qualification
**	dml_level	{nat}  DML compatibility level, e.g., UI_DML_QUEL.
**	oneonly		Flag - if TRUE, only one qualification per field is
**				allowed.
**	prefix		A character string to send before any
**			thing else.  If NULL, don't send.
**
**	func		A function to call on each query spec.
**
**	func_param	A parameter to pass the function given by func.	 This
**			value is passed to func with a QRYSPEC.
** Returns:
**	{STATUS}		OK if the qualification is valid.
**				QG_NOSPEC
**				    if no QRY_SPECS were generated.
**				specific value if there is an error that
**				    has an associated error message.
**				FAIL if there is a general error.
**				    An error message will have been
**				    displayed explaining specifically
**				    what the problem was.
**
** History:
**	23-mar-1987	Added header.  Modified extensively for 6.0 (drh)
**	4-may-1987 (Joe)
**		Changes for function call to QG.
**	09/29/88 (dkh) - Changed to pass in large buffer for string
**			 datatypes.
**	28-jun-90 (kevinm)  Don't skip leading white space if only one
**		qualification is accepted in the field.  Bug 21320.
*/

STATUS
FDrngchk(frm, fname, tfcolname, source, build, tblname, colname, oneonly,
dml_level, prefix, func, func_param)
FRAME	*frm;
char	*fname;
char	*tfcolname;
DB_DATA_VALUE *source;
bool	build;
char	*tblname;
char	*colname;
bool	oneonly;
i4	dml_level;
char	*prefix;
STATUS	(*func)();
PTR	func_param;
{
	FMT		*fldfmt;
	DB_DATA_VALUE	*flddbv;
 	DB_DATA_VALUE	chardbv;
	DB_TEXT_STRING	*txtptr;
	STATUS		rval;
	char		selector[2*FE_MAXNAME+1+1];
 	i4		class;
 	AFE_DCL_TXT_MACRO(DB_GW4_MAXSTRING)	chardata;

	if ( source == NULL )
	{
		return QG_NOSPEC;
	}

	if ( source->db_datatype != DB_LTXT_TYPE || source->db_data == NULL )
	{
		return FDRQLTXT;
	}

	/*
	**  Point to the query string in the display buffer (or equivalent).
	**  This is the value that will be parsed.
	*/

	txtptr = (DB_TEXT_STRING *) source->db_data;
	FDsource = (char *) txtptr->db_t_text;
	FDnsleft = txtptr->db_t_count;

	/*
	**  If there is nothing to parse, return TRUE.
	*/

	skip_white();		/* skip any leading blanks */
	if ( FDnsleft <= 0 )
	{
		return QG_NOSPEC;
	}

	/* if only one qualification don't skip leading white space. */
 	if ( oneonly )
 	{ /* restore leading white space */
 		FDsource = (char *) txtptr->db_t_text;
 		FDnsleft = txtptr->db_t_count;
 	}

 	/* Initialize statics. */
	if ( memTag == 0 )
	{
		memTag = FEgettag();
		if ( (adfCb = FEadfcb()) == NULL )
			return FAIL;
	}

	/*
	**  Set up default return values.
	*/

	FDbuild = build;
	if ( FDbuild )
	{
		if (func == NULL)
			return FDRQNULL;	/* shouldn't happen */
		qgfunc = func;
		qgparam = func_param;
	}

	/*
	**  Get the field's DB_DATA_VALUE for its internal value.  This is
	**  needed to perform datatype validity checking.  Also get the format.
	*/

	if ( (rval = FDdbvget( frm, fname, tfcolname, 0, &flddbv )) != OK ||
		(rval = FDfmtget( frm, fname, tfcolname, 0, &fldfmt )) != OK )
	{
		return rval;
	}

	FDq_lang = dml_level;
	FDgotoprnd = FALSE;
	MEfill( stnleft = MAXST, EOS, stnext = stbuf );

	sqlpat = PM_NOT_FOUND;
	pm_cmd = PM_NO_CHECK;

	if ( !FDbuild )
	{
		FDrcksel = NULL;
		FDrslen = 0;
	}
	else
	{
		if ( tblname != NULL)
		{
			STcopy(tblname, selector);
			STcat(selector, ERx("."));
		}
		else
		{
			selector[0] = EOS;
		}
		if ( colname != NULL )
		{

			STcat(selector, colname);
		}
		FDrcksel = selector;
		FDrslen = STlength( FDrcksel );
		/*
		** Send down the prefix
		*/
		if ( prefix != NULL )
		{
			QRY_SPEC	qspec;

			qspec.qs_var = (PTR) prefix;
			qspec.qs_type = QS_TEXT;
			if ( (rval = (*qgfunc)(qgparam, &qspec)) != OK )
				return rval;
		}
 	}
 
 	/*
 	**  Pass down big buffer if the datatype is one of the
 	**  character types.  This way, user can enter a patten
 	**  that exceeds the normal data size for the field.
 	*/
 
 	IIAFddcDetermineDatatypeClass(flddbv->db_datatype, &class);
 	if (class == CHAR_DTYPE)
 	{
 		MEcopy((PTR) flddbv, (u_i2) sizeof(DB_DATA_VALUE),
			(PTR)&chardbv);
 		chardbv.db_data = (PTR) &chardata;
		/*
		**  If the following length is set to DB_GW4_MAXSTRING, which
		**  some folks might think is the proper value, some gateways
		**  may choke on it.  Let's keep it smaller.  If the user
		**  types in more than DB_MAXSTRING characters they are
		**  asking for trouble. (kenl)
		*/
 		chardbv.db_length = DB_MAXSTRING;

 		flddbv = &chardbv;
	}

	/*
	**  Parse the qualification string.
	*/

	rval = rngparse( flddbv, fldfmt, 0, oneonly );
	if ( memTag != 0 )
		FEfree( memTag );

	return rval;
}

/*{
** Name:	rngparse() -	Parse a range query specification
**
** Description:
**	Calls itself recursively to parse a range query specification for
**	QBF or 4GL qualification specification.
**
**	This routine is internal to FRAME.
**
** Inputs:
**	flddbv		Ptr to the DB_DATA_VALUE for the field's internal
**			value.
**	fldfmt		Ptr to the field's format
**	level		Recursion level
**	oneonly		Flag - if TRUE, only one qualification is allowed
**			in the field.
**
** Returns:
**	{STATUS}	OK	Range query parsed sucessfully
**			FAIL	Error found in range query specification
**
** History:
**	03-apr-1987	Created (drh).
*/
static STATUS
rngparse ( flddbv, fldfmt, level, oneonly )
DB_DATA_VALUE	*flddbv;
FMT		*fldfmt;
i4		level;
bool		oneonly;	/* TRUE if only one qual allowed */
{
	i4	ntoskip = 0;
	i4	toktype;
	i4	oprndtype;
	i4	state;
	i4	prevtok = 0;
	bool	gettoken = TRUE;
	STATUS	rval;

	state = NDOPER;

	for (;;)
	{
		if ( gettoken )
		{
			prevtok = toktype;
			if ( (toktype = gettok( oneonly, &oprndtype, &ntoskip ))
					== BADTOKEN )
				return FAIL;
			gettoken = FALSE;
		}

		switch (state)
		{

		  case NDOPER:
			/*  Looking for an operator */

			switch ( toktype )
			{
			  case LPAREN:
				if ((rval=addtostr( FDsource, ntoskip )) != OK)
					return rval;
				FDsource += ntoskip;
				FDnsleft -= ntoskip;

				if ( (rval = rngparse( flddbv, fldfmt, level+1,
					oneonly)) != OK)
				{
					return( rval );
				}

				/*
				**  An operand or operator is required
				**  (i.e. "()" or "(())" is illegal )
				*/

				if ( !FDgotoprnd )
				{
					IIUGerr(E_FD0017_No_operand_fou,
						UG_ERR_ERROR, 0
					);
					return( FAIL );
				}
				state = NDRPAREN;	/* need right paren */
				gettoken = TRUE;
				break;

			  case OPER:
				if ((rval=addtostr( FDrcksel, FDrslen )) != OK)
					return rval;
				FDsource += ntoskip;
				FDnsleft -= ntoskip;
				state = NDOPRND;
				gettoken = TRUE;
				FDgotoprnd = TRUE;
				break;

			  case NOTOK:
				if ( prevtok == ANDOR )
				{
					/* error */
					return( FAIL );
				}
				/*  Nothing left to do */
				return putstr();
				break;

			  case OPERAND:
				/*  default to '=' for operator */
				if ((rval=addtostr( FDrcksel, FDrslen )) != OK)
					return rval;
				loper = _Equal;
			 	pm_cmd = PM_CHECK;
				state = NDOPRND;
				break;

			  case RPAREN:
				if ( level > 0 )
				{
					return putstr();
				}
				else	/* unmatched right paren */
				{
					IIUGerr(E_FD0018_unmatched_righ,
						UG_ERR_ERROR, 0
					);
					return FAIL;
				}

			  default:
				IIUGerr(E_FD0019_excess_charact,
					UG_ERR_ERROR, 0
				);
				return FAIL;

			} /* end of switch on toktype*/

			break;		/* end of NDOPER case */

		  case NDOPRND:
			if ( toktype == OPERAND )
			{
				if ( (rval = makedbv( flddbv, fldfmt, FDsource,
					ntoskip, oprndtype )) != OK)
				{
					IIUGerr(E_FD001A_bad_datatype,
						UG_ERR_ERROR, 0
					);
					return rval;
				}
				FDsource += ntoskip;
				FDnsleft -= ntoskip;
				FDgotoprnd = TRUE;
				gettoken = TRUE;
			}
			else
			{ /* use system default */
				if (FDbuild)
				{
					if ( (rval = adc_getempty(adfCb, flddbv)) != OK ||
							(rval = nullhdlr(flddbv)) != OK )
					{
						IIUGerr(E_FD001B_unable_to_crea,
							UG_ERR_ERROR, 0
						);
						return rval;
					}
				}
			}
			state = NDBOOL;
			break; /* end of NDOPRND case */

		case NDRPAREN:
			if ( toktype != RPAREN )
			{
				IIUGerr(E_FD001C_No_closing_par,
					UG_ERR_ERROR, 0);
				return( FAIL );
			}
			else
			{
				if ((rval=addtostr( FDsource, ntoskip )) != OK)
					return rval;
				FDsource += ntoskip;
				FDnsleft -= ntoskip;
				gettoken = TRUE;
				state = NDBOOL;
			}
			break;

		case NDBOOL:
			if (oneonly && toktype != NOTOK)
			{
				IIUGerr(E_FD001D_excess_charact,
					UG_ERR_ERROR, 0
				);
				return FAIL;
			}

			switch (toktype )
			{
			  case ANDOR:
				/* got what we wanted */
				if ((rval=addtostr( FDsource, ntoskip )) != OK)
					return rval;
				FDsource += ntoskip;
				FDnsleft -= ntoskip;
				gettoken = TRUE;
				state = NDOPER;
				break;

			  case RPAREN:
				if ( level > 0 )
				{
					putstr();
					return (OK);
				}
				else	/* unmatched right paren */
				{
					IIUGerr(E_FD001E_unmatched_righ,
						UG_ERR_ERROR, 0
					);
					return FAIL;
				}
				break;

			  case NOTOK:
				return putstr();/* write any remaining strs */
				break;

			  default:

				/*
				**  Got something, but not AND or OR.  Default
				**  to AND.
				*/

				if ( (rval=addtostr(_And, sizeof(_And)-1 )) != OK )
					return rval;
				gettoken = FALSE;
				state = NDOPER;
				break;
			} /* end of switch on toktype */
			break;	/* end of NDBOOL case */

			default:
			  /*
			  **  This should never happen - state should always be
			  **  defined legally.
			  */
			  IIUGerr(E_FD001F_unrecognized, UG_ERR_ERROR, 0);
			  return(FAIL);
			  break;
		} /* end of switch on state */
	} /* end of for loop */
}

/*
** Name:	gettok	-	Get next token in field qualification
**
** Description:
**	Gets the next token in FDsource, and return to
**	the caller the token type, the number of bytes in it, and whether
**	it is a quote-delimited operand.  Skips any leading blanks in the
**	buffer before tokenizing.  Recognized token types are:
**		left parenthesis
**		right parenthesis
**		'and' or 'or'
**		operators (i.e. the comparison operators )
**		operands (i.e. the value to compare to)
**
**	If there are no more tokens to parse (i.e. after skipping leading
**	blanks, FDnsleft is zero) returns NOTOK.
**
**
** Inputs:
**	oneonly	{bool}  Only recognize a single operator and operand.
**
** Outputs:
**	oprndtype	If token type is OPERAND, set to either OP_NODELIM if
**			   the operand is not delimited with quotes, or
**			   OP_DQUOT if the operand is delimited with double
**			   quotes.
**	toksize		Updated with the number of bytes in the token.
**
** Returns:
**	{nat}		Returns the token type of the 'next' token
**			   in the buffer.
**
** History:
**	26-mar-1987	Created (drh).
**	21-sep-1988 (jhw)  Added support for parsing only one value.
**	28-jun-90 (kevinm)  Don't skip leading white space if only one
**		qualification is accepted in the field.  Bug 21320.
*/

static i4
gettok ( oneonly, oprndtype, toksize )
bool	oneonly;
i4	*oprndtype;
i4	*toksize;
{
	*oprndtype = 0;
	*toksize = 0;

 	if ( !oneonly )
 		skip_white();	/* skip leading white space */
 
	if ( FDnsleft <= 0 )
	{
		return NOTOK;
	}

	if ( *FDsource == '(' && !oneonly )
	{
		CMbyteinc( *toksize, FDsource );
		return LPAREN;
	}
	else if ( *FDsource == ')' && !oneonly )
	{
		CMbyteinc( *toksize, FDsource );
		return RPAREN;
	}
	else if ( isoper( toksize ) )
	{
		if ( loper == _Equal || loper == _UnEqual )
		{
			pm_cmd = PM_CHECK;
		}
		return OPER;
	}
	else if ( !oneonly && isandor( toksize ) )
	{
		return ANDOR;
	}
	else if ( (getoprnd( oneonly, toksize, oprndtype )) == OK )
	{
		return OPERAND;
	}
	else
	{
		return BADTOKEN;
	}
	/*NOTREACHED*/
}

/*
** Name:	isoper	-	Is next token an operator?
**
** Description:
**	Determine whether the next token in the FDsource buffer is an
**	operator (i.e. a comparison operator like =, >, etc).  If it
**	is return TRUE, and the number of bytes in the operator,
**	otherwise return FALSE.
**
** Inputs:
**	nbytes		Ptr to a i4
**
** Outputs:
**	nbytes		Updated with the number of bytes in the token if
**			it is an operator, or zero.
**
** Returns:
**	bool		TRUE if next token is an operator
**			FALSE if next token is not an operator
**
** Exceptions:
**	none
**
** Side Effects:
**	Uses FDsource as the buffer to check, and FDnsleft to determine
**	how many bytes are left in the FDsource buffer.
**
** History:
**	26-mar-1987	Created (drh).
*/

/*}
** Name:	OPDESC		-	Operator descriptor
**
** Description:
**	Describes an operator in the 'range' language used by QBF in
**	query mode.
**
** History:
**	24-mar-1987	Created (drh).
*/

typedef struct
{
	i4	nbytes;
	char	*oper;
} OPDESC;

/*
**  Note that in the OPDESC array, it is ESSENTIAL that longer operator
**  strings precede shorter ones (particularly for cases where a long and
**  short one have a common first character - otherwise we would think that
** '>=' was '>' )
*/

static
	OPDESC quelopers[] =
{
	{ sizeof(_NotEqual)-1,	_NotEqual},
	{ sizeof(_GreatrEqual)-1,	_GreatrEqual},
	{ sizeof(_LessEqual)-1,	_LessEqual},
	{ sizeof(_UnEqual)-1,	_UnEqual},
	{ sizeof(_Equal)-1,		_Equal},
	{ sizeof(_Greater)-1,	_Greater},
	{ sizeof(_Less)-1,		_Less},
	{ 0, NULL }
};

static bool
isoper( nbytes )
i4	*nbytes;
{
	register OPDESC	*opptr;

	*nbytes = 0;

	if ( FDnsleft <= 0 )
		return FALSE;

	for ( opptr = quelopers ; opptr->nbytes != 0 ; ++opptr )
	{
		if ( frsubstr(opptr->oper, FDsource) )
		{
			*nbytes = opptr->nbytes;

			/* Map "!=" to "<>" */
			loper = ( opptr->oper == _NotEqual )
					? _UnEqual : opptr->oper;
			return TRUE;
		}
	}
	return FALSE;
}

/*
** Name:	ismetachar() -	Is the character a metacharacter (MACRO)
**
** Description:
**	Checks to see if the character is a metacharacter.  This routine
**	does NOT consider whitespace, a single quote, or a double quote to be
**	a metacharacter.
**
**	It is query language sensitive in that if the query language is NOT
**	QUEL, it considers '^' to be a metacharacter.
**
** Inputs:
**	p	{char *}  String containing character to check.
**
** Returns:
**	{bool}	TRUE if the character is a metacharacter
**		FALSE otherwise
**
** History:
**	20-mar-1987	Created (drh)
*/

#define	ismetachar( p ) ( *(p) == '(' || *(p) == ')' || \
				*(p) == '=' || *(p) == '>' || \
				*(p) == '<' || *(p) == '!' || \
				( FDq_lang != UI_DML_QUEL && *(p) == '^' ) \
			)

/*
** Name:	isandor() -	Is next token 'and' or 'or'?
**
** Description:
**	Check whether the next token in FDsource is 'and' or 'or'.  If
**	so, return TRUE and the number of bytes in the token, otherwise
**	return FALSE.
**
** Inputs:
**	Ptr to a i4  to update
**
** Outputs:
**	ntoskip		Updated with the number of bytes in the token if it
**			is 'and' or 'or', or zero otherwise.
**
** Returns:
**	{bool}	TRUE if next token is 'and' or 'or'
**		FALSE otherwise
**
** History:
**	27-mar-1987	Created (drh)
*/
static bool
isandor( ntoskip )
i4	*ntoskip;
{
	char	*strptr = FDsource;

	*ntoskip = 0;

	/*
	**  See if it's an AND
	*/

	if ( frsubstr(_And, strptr) )
	{
		*ntoskip = 3;
	}
	else if ( frsubstr(_Or, strptr) )
	{
		*ntoskip = 2;
	}
	else
		return FALSE;

	/*
	**  AND or OR was matched - must be last in string, or followed by
	**  white space or a metachar otherwise it's the start of an
	**  operand
	*/

	if (  (FDnsleft - *ntoskip ) == 0 )
		return TRUE;		/* last token in string */

	strptr += *ntoskip;		/* point past AND or OR */

	if ( CMwhite( strptr ) || ismetachar( strptr ) )
		return TRUE;
	else
	{
		*ntoskip = 0;
		return FALSE;
	}
}

static STATUS
getoprnd( oneonly, ntoskip, oprndtype )
bool	oneonly;
i4	*ntoskip;
i4	*oprndtype;
{
	register i4	type = 0;
	register i4	nleft = FDnsleft;
	register char	*opptr = FDsource;
	register char	*prevchar = NULL;
	bool		eot = FALSE;

	*oprndtype = 0;
	if ( nleft <= 0 )
	{
		*ntoskip = 0;
		return OK;
	}

	if ( !oneonly )
	{
		type = ( *opptr == '"' ) ? OP_DQUOT : OP_NODELIM;

		if ( type != OP_NODELIM )
		{ /* need to skip leading quote */
			CMbytedec( nleft, opptr );
			prevchar = opptr;
			CMnext( opptr );
		}
	}

	while ( nleft > 0 )
	{
		if ( type == OP_DQUOT && *opptr == '"' && *prevchar != '\\' )
		{
			eot = TRUE;
			CMbytedec( nleft, opptr );    /* skip terminal quote */
			break;		/* end of double quoted string */
		}
		else if ( type == OP_NODELIM && ( CMwhite( opptr ) ||
				ismetachar( opptr ) ) )
			break;
		else
		{
			prevchar = opptr;
			CMbytedec( nleft, opptr );
			if ( nleft > 0 )
				CMnext( opptr );
		}
	} /* end of while loop */

	/*
	**  If the operand was a quoted string, check that it was terminated
	**  properly (i.e. not by running out of string).  If it wasn't,
	**  print an error that the quoted string was not terminated.
	*/

	if ( type == OP_DQUOT && !eot )
	{
		IIUGerr(E_FD0020_unterminated, UG_ERR_ERROR, 0);
		return FAIL;
	}

	/*
	**  If we get here and the number of bytes to skip is zero,
	**  then we did not find a good operand.  Null values that
	**  are expressed as the empty string may be a problem if
	**  the following syntax is enterd "= !=".  Since this is
	**  not correct syntax anyway, I will let this case go.
	*/
	if ((*ntoskip = FDnsleft - nleft) == 0)
	{
		IIUGerr(E_FD0025_Syntax_error, UG_ERR_ERROR, 0);
		return FAIL;
	}
	*oprndtype = type;

	return OK;
}

/*{
** Name:	skip_white()
**
** Description:
**	Skip blank spaces in a qbf field qualification.
**
**	This routine is INTERNAL to FRAME.
**
** Inputs:
**
** Outputs:
**
** Returns:
**	STATUS
**
** Exceptions:
**	none
**
** Side Effects:
**	Uses the qualification in FDsource, with FDnsleft indicating how
**	many bytes are left.  Both of these will be updated to point
**	to the next non-blank, with FDnsleft decremented appropriately.
**
** History:
**	23-mar-1987	Added header, and modified to use CM routines.
*/

static VOID
skip_white()
{
	while ( FDnsleft > 0 && CMwhite( FDsource ) )
	{
		CMbytedec( FDnsleft, FDsource );
		CMnext( FDsource );
	}
}

/*{
** Name:	addtostr	-	Add to ongoing query string
**
** Description:
**	Add the current token to the query string that is in the
**	process of being built.	 This is never done for operands - those
**	are handled as DB_DATA_VALUES - but is done for the operators,
**	parentheses, 'and', and 'or'.
**
** Inputs:
**	strptr		Ptr to the token to add (NOT null terminated)
**	nbytes		No. of bytes in the token.  This is the number
**			that will be copied into the query string.
**
** Outputs:
**
** Returns:
**	STATUS of any call to putstr.
**
** Exceptions:
**	none
**
** Side Effects:
**	The token is copied into the static strbuf array.
**
** History:
**	24-mar-1987	Created (drh).
*/

static STATUS
addtostr( strptr, nbytes)
char	*strptr;
i4	nbytes;
{
	STATUS	rval;

	if ( nbytes <= 0 )
		return OK;

	if ( (nbytes + 1 ) > stnleft )
	{
		if ((rval = putstr()))
			return rval;
	}
	for ( ; nbytes > 0 ; --nbytes )
	{
		*stnext++ = *strptr++;
		--stnleft;
	}

	*stnext++ = ' ';
	--stnleft;
	return OK;
}

/*
** Name:	putstr() -	Add string to the qry_spec array
**
** Description:
**	Take the query string, build a qry_spec, and call the func
**	to pass the qry_spec to QG.
**
** Inputs:
**
** Outputs:
**
** Returns:
**	STATUS of call to qgfunc.
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	24-mar-1986	Created (drh)
**	4-may-1987 (Joe)
**		Changed to return STATUS and made to call QG function.
*/
static STATUS
putstr()
{
	char		*string;
	QRY_SPEC	qspec;
	i4		stsize;
	STATUS		rval;

	if ( !FDbuild || stnleft == MAXST )
		return OK;

	stsize = (MAXST - 1) - stnleft;
	if (stsize <= 0)
		return OK;
	stbuf[stsize] = EOS;
	qspec.qs_var = (PTR) stbuf;
	qspec.qs_type = QS_TEXT;
	if ((rval = (*qgfunc)(qgparam, &qspec)) != OK)
		return rval;
	stnext = stbuf;
	stnleft = MAXST - 1;
	STmove( ERx(" "), ' ', (i2) (MAXST - 1), stnext );

	return OK;
}

/*
** Name:	makedbv() -	Make DB_DATA_VALUE from operand.
**
** Description:
**	The scanner has found an operand.  This operand must be converted
**	to the internal value of the field using the field's format. If that
**	conversion fails, then the operand is not legal for the field's
**	datatype and format.  Otherwise, if it succeeds, the resulting
**	DB_DATA_VALUE is put into a QRY_SPEC which is then passed to
**	the qgfunc.
**
**	Note that the operand conversion will actually use the field's
**	DB_DATA_VALUE.
**	- this is assumed to be harmless.
**
** Inputs:
**	flddbv		Ptr to the field's DB_DATA_VALUE
**	fldfmt		Ptr to the field's format
**	oprnd		Ptr to start of the operand as chars
**	nbytes		No. of bytes in the operand
**	optype		Operand type (i.e. quote delim type)
**
** Returns:
**	{STATUS}	OK	Conversion succeeded.
**			FAIL	Operand could not be converted to internal
**				value.	An error message will have been
**				displayed.
**
** History:
**	25-mar-1987	Created (drh).
**	19-nov-87 (kenl)
**		Changed call to fmt_cvt to fcvt_sql.  The new routine is the
**		same as fmt_cvt with the addition of a new parameter, sqlpat,
**		which is set to TRUE when a QUEL pattern matching character
**		is changed to its SQL equivalent.
*/
static STATUS
makedbv( flddbv, fldfmt, oprnd, nbytes, optype )
DB_DATA_VALUE	*flddbv;
FMT		*fldfmt;
char		*oprnd;
i4		nbytes;
i4		optype;
{
	STATUS		retstat = OK;
	DB_DATA_VALUE	*dbvptr;
	u_i4		size;
	i4		pat_temp;
	DB_TEXT_STRING	*txtdata;
	QRY_SPEC	qspec;
	EX_CONTEXT	context;
	DB_DATA_VALUE	oprnddbv;	/* DB_LTXT_TYPE dbv for operand */
	bool		reversed;

	/*
	**  Set up a DB_DATA_VALUE to hold the operand as a DB_LTXT_TYPE
	*/

	oprnddbv.db_datatype = DB_LTXT_TYPE;
	size = sizeof( DB_TEXT_STRING ) + nbytes;
	oprnddbv.db_length = size;
	oprnddbv.db_prec = 0;
	txtdata = (DB_TEXT_STRING *)FEreqmem( memTag, size, TRUE, &retstat );

	/*
	**  If operand was quoted, leave the quotes off
	*/

	if ( optype == OP_DQUOT )
	{
		nbytes -= 2;			/* decrement for quotes */
		CMnext( oprnd );		/* point past leading quote */
	}
	txtdata->db_t_count = nbytes;
	MEcopy( (PTR) oprnd, (u_i2) nbytes, (PTR) txtdata->db_t_text );
	oprnddbv.db_data = (PTR) txtdata;

	dbvptr = flddbv;

	/*
	**  Convert to internal value for field
	*/

	/*
	**  Before converting, set up exception handler to catch
	**  overflows.
	*/
	IIFDftFldTitle[0] = EOS;
	IIFDfinFldInternalName[0] = EOS;
	if ( EXdeclare(FDadfxhdlr, &context) != OK )
	{
		EXdelete();
		return FAIL;
	}

	/* Fix for bug 2624.  Fix the kludge when time permits. */
	_VOID_ fmt_isreversed(adfCb, fldfmt, &reversed);
	if (reversed)
	{
		fldfmt->fmt_var.fmt_reverse = FALSE;
	}
	retstat = fcvt_sql( adfCb, fldfmt, &oprnddbv, dbvptr, (bool) TRUE,
 			( FDq_lang == UI_DML_QUEL ) ? DB_QUEL : DB_SQL,
 			( FDq_lang == UI_DML_GTWSQL ) ? PM_GTW_CHECK : PM_CHECK,
 			&sqlpat
	);
	if (reversed)
	{
		fldfmt->fmt_var.fmt_reverse = TRUE;
	}
	/* End of fix for bug 2624.  Fix the kludge when time permits. */

	EXdelete();

	if ( pm_cmd == PM_NO_CHECK &&
		( sqlpat == PM_FOUND || sqlpat == PM_USE_ESC ) )
	{
		IIUGerr(E_FD0026_rel_used_with_pattern, UG_ERR_ERROR, 0);
		sqlpat = PM_NOT_FOUND;
		return FAIL;
	}
 	else if ( FDq_lang == UI_DML_GTWSQL && sqlpat == PM_USE_ESC
			&& !IIUIdce_escape() )
	{
 		IIUGerr( E_FD0027_illegal_gateway_query, UG_ERR_ERROR,
				1, ERx("%")
		);
		sqlpat = PM_NOT_FOUND;
		return FAIL;
	}

	if (retstat)
		return(retstat);
	else if (FDbuild)
		return(nullhdlr(dbvptr));
	else
		return(OK);
}

/*
** Name:	nullhdlr - Handle null values.
**
** Description:
**	Routine handles outputting correct query values even if the
**	DB_DATA_VALUE contains null.  If the value, is null, must
**	check the operator that was specified.	Only "=" and "!="
**	are allowed to be used with a NULL value.  Since we can
**	not send a null value directly, must transform to "IS NULL"
**	or "IS NOT NULL" as well.
**
** Inputs:
**	dbv	{DB_DATA_VALUE *}  Reference to DB_DATA_VALUE to process.
**
** Returns:
**	{STATUS}  OK	If processing was successful.
**		  FAIL	If bad operators were found or output failed.
** History:
**	07/24/87 (dkh) - Created for fix to jup bug 522.
*/
static STATUS
nullhdlr(dbv)
DB_DATA_VALUE	*dbv;
{
	STATUS		stat = OK;
	i4		isnull;
	DB_TEXT_STRING	*txt;
	DB_DATA_VALUE	sdbv;

	/*
	**  First do check to see if operand is the NULL value
	**  or not.  If so, only valid with "=" and "!=" operators.
	**  In addition, we need to generate the appropriate
	**  "IS NULL" or "IS NOT NULL" clause instead.
	*/
	if ( (stat = adc_isnull(adfCb, dbv, &isnull)) != OK )
	{
		return stat;
	}

	if (isnull)
	{ /* Null value */
		/*
		**  We have a null value, must check to make sure
		**  operator is correct and then put out either
		**  "IS NULL" or "IS NOT NULL".
		*/
		if ( loper == _Equal )
		{
			/* Add "IS NULL" and then output */
			if ( (stat = addtostr( _IsNull, sizeof(_IsNull)-1 ))
					!= OK || (stat = putstr()) != OK )
			{
				return stat;
			}
		}
		else if ( loper == _UnEqual )
		{
			/* Add "IS NOT NULL" and then output */
			if ( (stat = addtostr( _IsNotNull, sizeof(_IsNotNull)-1 ))
					!= OK || (stat = putstr()) != OK )
			{
				return stat;
			}
		}
		else
		{
			IIUGerr(E_FD0021_May_only_use, UG_ERR_ERROR, 0);
			stat = FAIL;
		}
	}
	else
	{
	    DB_DATA_VALUE	dbv_temp;
	    QRY_SPEC		qryspec;

	    /*
	    **  Output the operator first.
	    */

	    /* trim trailing blanks */
	    if ( (stat = IIftrim(dbv, &dbv_temp)) != OK )
	    {
	        return stat;
	    }

	    if (dbv_temp.db_datatype == DB_VCH_TYPE ||
		dbv_temp.db_datatype == DB_TXT_TYPE)
	    {
		txt = (DB_TEXT_STRING *) dbv_temp.db_data;
		if ((dbv_temp.db_length = txt->db_t_count) == 0)
		{
			dbv_temp.db_length = 10;
		}
		else
		{
			if ((stat = adc_lenchk(adfCb, TRUE, &dbv_temp,
				&sdbv)) != OK)
			{
		    		return(stat);
			}
			dbv_temp.db_length = sdbv.db_length;
		}
	    }

	    /*
	    **	If a QUEL pattern matching character was changed to its
	    **	SQL equivalent then we are in a position to use the LIKE
	    **	clause within an SQL SELECT statement.  NOTE: If a percent
	    **	sign existed in the user query AND NO QUEL PATTERN MATCHING
	    **	CHARACTERS WERE USED, sqlpat will be FALSE and the query
	    **	will be sent to QG using '='.
	    */
	    if ( sqlpat == PM_FOUND || sqlpat == PM_USE_ESC )
	    {
		if ( loper == _UnEqual )
		{ /* NOT LIKE */
		    if ( (stat = addtostr( _NotLike, sizeof(_NotLike)-1 ))
				!= OK )
		    {
			return stat;
		    }
		}
		/* must be LIKE */
		else
		{
		    if ( (stat = addtostr( _Like, sizeof(_Like)-1 )) != OK )
		    {
			return stat;
		    }
		}
	    }
	    else
	    {
		/* Send "!=" for QUEL */
		char	*op = ( loper != _UnEqual || FDq_lang != UI_DML_QUEL )
					? loper : _NotEqual;
		/*
		** If no pattern matching characters were found then
		** send to QG whatever operator was found in the user query.
		*/
		if ( (stat = addtostr(op, STlength(op))) != OK )
		{
		    return stat;
		}
	    }

	    /*
	    **  Now send the operator to QG.
	    */
	    if ( (stat = putstr()) != OK )
	    {
		return stat;
	    }

	    qryspec.qs_var = (PTR) &dbv_temp;
	    qryspec.qs_type = (sqlpat != PM_QUEL_STRING) ? QS_VALUE : QS_QRYVAL;

	    if ( (stat = (*qgfunc)(qgparam, &qryspec)) != OK )
	    {
		return stat;
	    }

	    if (sqlpat == PM_USE_ESC)
	    {
		/* Add "ESCAPE '\'" and then output */
		if ( (stat = addtostr(_Escape, sizeof(_Escape)-1)) != OK ||
	        		(stat = putstr()) != OK )
		{
		    return stat;
		}
	    }
	}

	pm_cmd = PM_NO_CHECK;
	sqlpat = PM_NOT_FOUND;
	return stat;
}

/*
** Name:	frsubstr() - 	Case-less Prefix Comparison.
**
** Description:
**	Check to see if string A is a prefix of string B.  Always ignore case.
**	If B is shorter than A, then return FALSE.
**
** Inputs:
**	A	{char *}  Prefix to look for in string B.
**	B	{char *}  Base string in which to perform the lookup.
**
** Returns:
**	{bool}	TRUE	If A is a substring at the start of string B.
**		FALSE	If match failed.
**
** History:
**	07/24/87 (dkh) - Initial version.
*/
static bool
frsubstr ( stra, strb )
register u_char	*stra;
register u_char	*strb;
{
	while ( *stra != EOS && CMcmpnocase(stra, strb) == 0 )
	{
		CMnext(stra);
		CMnext(strb);
	}

	return (bool)( *stra == EOS );
}


/*{
** Name:	IIFDrcsRangeCheckSuffix - Send query suffix
**
** Description:
**	FDrngchk allows prefixes for pieces of the query string.  This
**	allows a suffix.  Qualifications in tablefields require this, since
**	there may be trailing parentheses.
**
** Inputs:
**	suffix		Suffix string to append to query
**
**	qgfunc		A function to call on each query spec.
**
**	qgfunc_param	A parameter to pass the function given by func.	 This
**			value is passed to func with a QRYSPEC.
**	
**
** History:
**	1/91 (Mike S) Initial version
*/
STATUS 
IIFDrcsRangeCheckSuffix(suffix, qgfunc, qgparam)
char	*suffix;
STATUS	(*qgfunc)();
PTR	qgparam;
{
	QRY_SPEC	qspec;

	qspec.qs_var = (PTR) suffix;
	qspec.qs_type = QS_TEXT;
	return (*qgfunc)(qgparam, &qspec);
}
