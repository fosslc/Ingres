/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**	Header file for the Output formatting routines.
**
**	HISTORY:
**	11/20/89 (elein) B8666 integrated 2/27/90
**		- Removed unused definitions for:
**		  MINS_HR, MINS_DAY, MINS_MON, MINS_YR, HRS_DAY, 
**		  HRS_MON, HRS_YR, DAYS_MON, DAYS_YR
**		- Moved to SECS per definitions to front!utils!hdr!dateadt.h
**		  and redefined SECS per definitions in terms of adu
**		  definitions.
**		- moved definition of WEEKLENGTH to dateadt.h and redefined
**		  in terms of adu constants.
**		* NOTE front!utils!hdr!dateadt.h includes
**		  common!hdr!hdr!adudate.h from the adt facility.
**		  It has THE DEFINITIVE date information which should
**		  be used sparingly
**	30-apr-90 (cmr) Added defines for F_inquote values.
**	08/16/91 (tom) support for input masking of dates, and string templates
**		this involves (amoung other things) moving some items up to
**		fmt.h in HDR2 so that the window manager code can see some
**		of the items.
**	7-dec-1993 (rdrane)
**		Add function declaration for f_cvta().
**	30-sep-96 (mcgem01)
**		externs changed to GLOBALREFs.
**	22-dec-1998 (crido01) SIR 94437
**	   	Add FC_ROUNDUP and FCP_ROUNDUP for decimal rounding template.
**      18-apr-2006 (stial01)
**              Added fields to FM_WORKSPACE
*/


#define DATEFMT 1	/* compile on for report writer */

/*
**	Bit numbers in fmt_prec for specified components of date template.
*/
# define	FD_LEN		7	/* # of bits used in fmt_prec */

# define	FD_ABS		0	/* absolute date specified */
# define	FD_YRS		1	/* years specified */
# define	FD_MONS		2	/* months specified */
# define	FD_DAYS		3	/* days specified */
# define	FD_HRS		4	/* hours specified */
# define	FD_MINS		5	/* minutes specified */
# define	FD_SECS		6	/* seconds specified */
# define	FD_INT		8	/* date interval specified */

/*
**	KYW - structured element of the keyword table for date template.
**
*/

typedef struct
{
	char	*k_name;	/* actual keyword name */
	char	k_class;	/* class of keyword */
	i4	k_value;	/* value of keyword within class */
} KYW;

/*
**	Values for k_class
*/

# define	F_MON		'm'	/* month */
# define	F_DOW		'd'	/* day of week */
# define	F_PM		'p'	/* pm or am */
# define	F_TIUNIT	'u'	/* time interval unit */
# define	F_ORD		'o'	/* ordinal number suffix */

	
/*
**	Constants Used by Formatting Routines.
*/

/*
**	Format codes for numeric templates 
*/

# define	FC_SEPARATOR	','	/* comma separator */
# define	FC_DECIMAL	'.'	/* decimal */
# define	FC_CURRENCY	'$'	/* floating currency symbol */
# define	FC_DIGIT	'n'	/* force a digit */
# define	FC_ZERO		'z'	/* floating zero */
# define	FC_MINUS	'-'	/* floating minus, on negative */
# define	FC_PLUS		'+'	/* floating sign */
# define	FC_STAR		'*'	/* check fill */
# define	FC_BLANK	' '	/* force a blank */
# define	FC_FILL		' '	/* fill character for templates */
# define	FC_ERROR	'*'	/* fill character in templates on error */
# define	FC_OPAREN	'('	/* floating parenthesis, on negative */
# define	FC_CPAREN	')'	/* floating parenthesis, on negative */
# define	FC_OANGLE	'<'	/* floating angle bracket, on negative */
# define	FC_CANGLE	'>'	/* floating angle bracket, on negative */
# define	FC_OCURLY	'{'	/* floating curly bracket, on negative */
# define	FC_CCURLY	'}'	/* floating curly bracket, on negative */
# define	FC_OSQUARE	'['	/* floating square bracket, on negative */
# define	FC_CSQUARE	']'	/* floating square bracket, on negative */
# define	FC_ESCAPE	'\1'	/* escape character for '\' */
# define 	FC_ROUNDUP	'^'	/* Round remaing decimal up */

/*
**	String equivalents of format codes for numeric templates 
**	for use in ST[r]index.
*/

# define	FCP_SEPARATOR	ERx(",")	/* comma separator */
# define	FCP_DECIMAL	ERx(".")	/* decimal */
# define	FCP_CURRENCY	ERx("$")	/* floating currency symbol */
# define	FCP_DIGIT	ERx("n")	/* force a digit */
# define	FCP_ZERO	ERx("z")	/* floating zero */
# define	FCP_MINUS	ERx("-")	/* floating minus, on negative */
# define	FCP_PLUS	ERx("+")	/* floating sign */
# define	FCP_STAR	ERx("*")	/* check fill */
# define	FCP_BLANK	ERx(" ")	/* force a blank */
# define	FCP_FILL	ERx(" ")	/* fill character for templates */
# define	FCP_ERROR	ERx("*")	/* fill character in templates on error */
# define	FCP_OPAREN	ERx("(")	/* floating parenthesis, on negative */
# define	FCP_CPAREN	ERx(")")	/* floating parenthesis, on negative */
# define	FCP_OANGLE	ERx("<")	/* floating angle bracket, on negative */
# define	FCP_CANGLE	ERx(">")	/* floating angle bracket, on negative */
# define	FCP_OCURLY	ERx("{")	/* floating bracket, on negative */
# define	FCP_CCURLY	ERx("}")	/* floating bracket, on negative */
# define	FCP_OSQUARE	ERx("[")	/* floating bracket, on negative */
# define	FCP_CSQUARE	ERx("]")	/* floating bracket, on negative */
# define	FCP_ESCAPE	ERx("\1")	/* escape character for '\' */
# define	FCP_ROUNDUP	ERx("^")	/* Round decimal digits up */


/*
**	Values for F_inquote.
*/
# define	F_NOQ		0	/* not in a quote */
# define	F_INQ		1	/* in a quote */
# define	F_WRAPQ		2	/* in a wrapped quoted */
/*
**	Other constants 
*/

# define	MAXFORM		255	/* maximum length of format spec */

# define	MAXSETWIDTH	2	/* maximum width given to a single
					** digit by f_setfmt in determining
					** total width of template. */

# define	F_MONVALUE	2	/* index of 2nd month of year */

# define	F_DOWVALUE	3	/* index of 1st day of week */

# define	F_PMVALUE	12	/* index of "pm" */


GLOBALREF	KYW	*F_Abbrmon;
GLOBALREF	KYW	*F_Fullmon;
GLOBALREF	KYW	*F_Abdow;
GLOBALREF	KYW	*F_Abbrdow;
GLOBALREF	KYW	*F_Fulldow;
GLOBALREF	KYW	*F_Abbrpm;
GLOBALREF	KYW	*F_Fullpm;
GLOBALREF	KYW	*F_Ordinal;
GLOBALREF	KYW	*F_Fullunit;
GLOBALREF	KYW	*F_SFullunit;

GLOBALREF	i4	F_monlen;	/* length of 2nd month name
				** (i.e. "February") */
GLOBALREF	i4	F_monmax;	/* length of longest month name
				** (i.e. "September") */
GLOBALREF	i4	F_dowlen;	/* length of 1st day of week name
				** (i.e. "Sunday") */
GLOBALREF	i4	F_dowmax;	/* length of longest day of week name
				** (i.e. "Wednesday") */
GLOBALREF	i4	F_pmlen;	/* length of "pm" */

FUNC_EXTERN	i4	f_getword();
FUNC_EXTERN	i4	f_getnumber();
FUNC_EXTERN	bool	f_dkeyword();
FUNC_EXTERN	i4	f_formnumber();
FUNC_EXTERN	i4	f_formword();
FUNC_EXTERN	i4	f_dayofyear();
FUNC_EXTERN	i4	f_daysince1582();

FUNC_EXTERN	STATUS	f_fmtdate();
FUNC_EXTERN	STATUS	f_format();
FUNC_EXTERN	char	*f_gnum();
FUNC_EXTERN	STATUS	f_nt();
FUNC_EXTERN	STATUS	f_setfmt();
FUNC_EXTERN	bool	f_dtcheck();
FUNC_EXTERN	VOID	f_dtsetup();
FUNC_EXTERN	STATUS	f_stcheck();
FUNC_EXTERN	STATUS	f_stsetup();
FUNC_EXTERN	STATUS	f_cvta();

/* number of overflow bytes while formatting with a multi-line format */
# define	FMT_OVFLOW	2

/*}
** Name:	FM_WORKSPACE	Workspace type for formatting multi-line formats
**
** Description:
**	This type is for the workspace needed to format a value with a
**	multi-line format.
**
** History:
**	7-jan-87 (grant)	created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
typedef struct
{
	i4	fmws_left;	/* the number of bytes left to format in the
				** output column
				*/
	i4	fmws_count;	/* the number of bytes remaining in the source
				** string
				*/
	u_char	*fmws_text;	/* points to the first unformatted byte
				** (the one following the last formatted byte)
				** in the remaining string.
				*/
	uchar	*fmws_cprev;
	DB_TEXT_STRING *fmws_multi_buf;
				/* handle to buffer for formatting one row of
				** a multi-line format          
				*/
	i4	fmws_multi_size;/* size of fmws_multi_buf not including the
				** two byte count of db_t_count
				*/
	uchar	fmws_quote;	/* double or single quote */
	bool	fmws_inquote;   /* keeps track of the state of quoted strings
				** that span multiple lines or are unclosed.
				*/
	bool	fmws_incomment; /* keeps track of state of comments that span
				** multiple lines or are unclosed
				*/
} FM_WORKSPACE;
