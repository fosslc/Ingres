/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>		/* 6-x_PC_80x86 */
#include	<cm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<st.h>
#include	<er.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>

/*
** SCAN_NUM_BUF_LEN is the length in bytes of the largest ASCII 
** representation of a nat.  The conversion buffer, number[], is 
** declared as SCAN_NUM_BUF_LEN + 1 to accomodate the trailing
** EOS.  The conversion buffer holds the type's precision during 
** conversion from ASCII to i4  (e.g., char(25), where '25' will 
** be CVan'd).  
**
** SCAN_NUM_LOOP_CNT is the maximum number of times to loop
** while filling the buffer.  
**
** This removes an implicit assumption that numbers will only be 
** a single byte (see comments in iiafScanNum() below).  CMcpyinc() 
** already handles cases for double-byte copying, so on a platform 
** where numbers are two bytes long the value of SCAN_NUM_BUF_LEN 
** would be set to 20 and SCAN_NUM_LOOP_CNT left at 10.
**
*/
#define SCAN_NUM_BUF_LEN	10
#define SCAN_NUM_LOOP_CNT	10

/* NO_OPTIM=dgi_us5 */

/**
** Name:	aftychk.c -   Check a User Type Specification Module.
**
** Description:
**	Contains routines used to parse and verify a user type specification.
**	Defines:
**
**	afe_tychk()	check a user type specification.
**
**	Local to AFE module:
**	iiafParseType()	parse a type specification into a type name and length.
**	iiafScanNum()	scan a number in a type specification.
**
** History:
**	Revision 6.0  87/01/20  daver
**	Initial revision.
**	13-apr-87	(daver)
**		Changed aftychk to make the type nullable via AFE_MKNULL_MACRO
**		if the dbut_kind field of the DB_USER_TYPE passed in is
**		DB_UT_NULL.
**	25-jun-87	(danielt) changed parameter to iiafParseType()
**		from &(user_type->dbut_name) to user_type->dbut_name
**
**	Revision 6.2  89/05  wong
**	Moved special acceptance of 'char', 'varchar', 'text', 'c' as single
**	character strings to 'iiafTypeHack()'.  JupBug #6165.
**
**	03-aug-1989	(mgw)
**		fixed for adc_lenchk() interface change.
**	19-feb-92 (leighb) DeskTop Porting Change: Added missing st.h
**	20-aug-92 (davel)
**		Changed length argument  in call to iiafTypeHack() to be a 
**		DB_DATA_VALUE, so that precision can be hacked as well.
**	09/01/92 (dkh) - Added IIAFfedatatype() so that FE code can have
**			 a central location to check if a datatype is
**			 supported by the FE tools.
**	10/07/92 (dkh) - Fixed IIAFfedataype() to handle NULLABLE types as well.
**	26-may-1996 (allmi01)
**		Added NO_OPTIM for dgi_us5 to correct datatypes problems encounted in
**		datatypes test suites (mainly sigvio's).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

DB_STATUS    adi_tyid();
DB_STATUS    adc_lenchk();

/*{
** Name:	afe_tychk() -   Check a User Type Specification.
**
** Description:
**	Checks the type as specifed by a user for correctness.  The type
**	specification is passed in as a string that the user entered (e.g.,
**	"text(20)") and whether it is to be Nullable.  This routine will parse
**	it into its components and verify that they are correct.  If it is a
**	multi-word name like "long varchar" than the words should be separated
**	by spaces.
**
** Inputs:
**	cb		{ADF_CB *}  A reference to the ADF control block.
**
**	user_type      	{DB_USER_TYPE *}  The user type specification.
**		.dbut_name	{char *}  String containing the type name.
**
**		.dbut_kind	If set to DB_UT_NULL, the db_datatype will
**				be nullable.
**
** Outputs:
**	dbv	{DB_DATA_VALUE *}
**		.db_datatype    {DB_DT_ID}  The ADF type ID corresponding to the
**				type string.  This will be Nullable (negative)
**				if the Nullability member of 'user_type' is set
**				to DB_UT_NULL.
**
**		.db_length 	{i4}  The internal length of the type
**				corresponding to the type string.
**
** Returns:
**	{DB_STATUS}	OK
**			 Also, status from:
**				'iiafParseType()'
**				'adi_tyid()'
**				'adc_lenchk()'
** History:
**	05/89 (jhw)  Moved special acceptance of 'char', 'varchar', 'text', 'c'
**		as single character strings to 'iiafTypeHack()'.  JupBug #6165.
**	03-aug-1989	(mgw)
**		adc_lenchk() interface changed to take DB_DATA_VALUE last
**		argument instead of i4.
*/
DB_STATUS
afe_tychk ( cb, user_type, dbv )
ADF_CB	 	*cb;
DB_USER_TYPE	*user_type;
DB_DATA_VALUE	*dbv;
{
	STATUS		rval;
	DB_DATA_VALUE	dblength;		/* internal length */
	ADI_DT_NAME	typename;

	/*
	** 'iiafParseType()' will parse the type and length from the type
	** specification string.  For example, "text(10)" becomes "text" and
	** "10".  Fixed-length types like "integer", "long varchar", or "date"
	** will have lengths of 0 to comply with the 'adc_lenchk()' interface.
	**
	** The type name will be lowercased for 'adi_tyid()'.
	*/
	if ( (rval = iiafParseType(cb, user_type->dbut_name, &typename, dbv))
			!= OK  ||
		(rval = adi_tyid(cb, &typename, &dbv->db_datatype))
			!= OK )
		return rval;

	/*
	** Note 'adc_lenchk()' will pass back the internal length,
	** but uses it for intermediate results, hence the indirection.
	*/

	/*
	** what is the debugging trace information protocol?
	SIprintf("AFE_TYCHK:checking length, dtype %d, dlen %d\n",
		dbv->db_datatype, dbv->db_length);
	*/

	if ( (rval = adc_lenchk(cb, TRUE, dbv, &dblength)) != OK )
		return rval;

	dbv->db_length = dblength.db_length;
	dbv->db_prec = dblength.db_prec;

	/* Set the Nullability if so specified */
	if ( user_type->dbut_kind == DB_UT_NULL )
	{
		AFE_MKNULL_MACRO(dbv);
	}

	return OK;
}

/*{
** Name:	iiafParseType() -	Parse a Type Specification into a
**						Type Name and Length.
** Description:
**	Given a type string as specified by a user, this routine parses it into
**	its name and length components.  If the type name passed in *is* a fixed
**	length type, this routine returns a length of 0, which will be set by
**	'adc_lenchk()' later.
**
** Inputs:
**	cb	{ADF_CB *}  A reference to the ADF control block.
**
**  	uname	{char *}  User type specification.  Would contain the
**		whole string "text(10)", for example.
**
** Outputs:
**	typename {ADI_DT_NAME *}  The type name extracted from the type
**		specification. For example, for the type "text(10)", it
**		would contain "text".
**
**	dbv	{DB_DATA_VALUE *}
**	    .db_length	The length extracted from the type specification. This
**			will be 0 if the type is a fixed length.
**
**	    .db_prec	The scale extracted from the type specification.  This
**			is set only if syntax of the specification is
**			"name(len,len)".  Otherwise .db_prec is set to 0.
**
** Returns:
**	{DB_STATUS}	OK	If successful.
**
**			E_AF600F_NO_CLOSE_PAREN	An open parentheses was used
**						w/o a corresponding close paren.
**
**			E_AF6010_XTRA_CH_PAREN	Extra characters followed a
**						close paren.
**
**			E_AF6011_XTRA_CH_IFCNUM	Extra characters followed the
**						number in a type of 'i', 'f',
**						or 'c'.
**
**			E_AF6012_BAD_DECL_SYNTAX Unexpected characters found at
**						end of type name.
**
**			E_AF6013_BAD_NUMBER	A error occured while trying to
**						extract a number from the type.
**
** History:
**	Written	2/6/87	(dpr)
**	11/89 (jhw) -- Added decimal support.
**	03-apr-90 (sandyd)
**      	Changed DB_LN_FRM_PR_MACRO reference to DB_PREC_TO_LEN_MACRO
**      	to match the name change that was made by JRB in dbms.h.
**	09-jun-92 (rdrane)
**		Changed naming of 6012 error from E_AF6012_BAD_TRAIL_CH to
**		E_AF6012_BAD_DECL_SYNTAX to get around mismatch w/ eraf.msg.
**		This is intended as a temporary change until the full decimal
**		support (which was backed out for 6.4MR2) can be properly
**		restored.
**	23-mar-1993 (rdrane)
**		Decimal and numeric datatypes do not always have a scale
**		component.  Regardless, they must still set the db_length
**		and db_prec via the DB_* macros or they will be declared
**		invalid (b49206).
*/

static VOID	_ScanName();

DB_STATUS
iiafParseType ( cb, uname, typename, dbv )
ADF_CB		*cb;
char		*uname;
ADI_DT_NAME	*typename;
DB_DATA_VALUE	*dbv;
{
	STATUS	rval;
	char	*instring;
	i4	preclen;
	i4	scale;
	bool	flag_scale;


	dbv->db_length = 0;
	dbv->db_prec = 0;
	instring = uname;

	/*
	** '_ScanName()' parses the type name out of the input string and places
	** a lower-cased copy of it in the output buffer.  The input string
	** reference is updated to point to the character following the type
	** name in the string.
	*/
 	_ScanName ( &instring, typename->adi_dtname );

#ifdef xDEBUG
	SIprintf("tybreak: post ScanName, strings are %s, %s\n",
		 instring,typename->adi_dtname);
#endif

	if  (*instring == '(')
	{
		/*
		**	syntax: name(len[,scale])
		**
		** A length and/or scale exists, so validate the value(s).
		** If no scale exists and the type is not decimal or numeric,
		** then set the db_length  to the specified length.
		** If the type is decimal or numeric, then set db_length and
		** db_prec using the appropriate macros.  Note that the scale
		** may be non-existent in this case.
		*/
		CMnext(instring);		/* advance past the '(' */
		preclen = 0;
		scale = 0;
		flag_scale = FALSE;
		/*
		** iiafScanNum takes the input string, and converts the char
		** version of the number it was pointing to into numeric form.
		** the number is returned in 'preclen', and the input string is
		** updated.
		*/
		if  (iiafScanNum(&instring, &preclen) != OK)
		{
			return(afe_error(cb,E_AF6013_BAD_NUMBER,0));
		}
		if  (*instring == ',')
		{
			/*
			** We have a scale, so skip the ',' and get the
			** second number
			*/
			CMnext(instring);
			if ((rval = iiafScanNum(&instring, &scale)) != OK)
			{
				return(afe_error(cb,E_AF6013_BAD_NUMBER,0));
			}
			flag_scale = TRUE;
		}
		if  ((!flag_scale) &&
		     (STequal(typename->adi_dtname,ERx("dec")) == 0) &&
		     (STequal(typename->adi_dtname,ERx("decimal")) == 0) &&
		     (STequal(typename->adi_dtname,ERx("numeric")) == 0))
		{
			/*
			** No scale was specified, and the type is not
			** DECIMAL/NUMERIC
			*/
			dbv->db_length = preclen;
		}
		else
		{
			/*
			** We had an explicit scale specification, an explicit
			** type specification of DECIMAL/NUMERIC, or both.
			*/
			dbv->db_length = DB_PREC_TO_LEN_MACRO(preclen);
			dbv->db_prec = DB_PS_ENCODE_MACRO(preclen, scale);
		}
		if (*instring != ')')
		{
			return(afe_error(cb,E_AF600F_NO_CLOSE_PAREN,0));
		}
		CMnext(instring);		/* advance past the ')' */
		while (CMwhite(instring))	/* strip trailing blanks */
		{
			CMnext(instring);
		}
		if  (*instring == EOS)
		{
			/*
			** normal case would be at EOS
			*/
			return(OK);
		}
		else
		{
			return(afe_error(cb,E_AF6010_XTRA_CH_PAREN,0));
		}
	}
	else if (*instring == EOS)
	{
		/*
		** any fixed length types or i, f, or c types
		** we now have to do our special case for i, f, or c.
		*/
		if  ((rval = iiafTypeHack(cb,typename->adi_dtname,dbv)) != OK)
		{
			return(rval);
		}
		return(OK);
	}
	else
	{
		return(afe_error(cb,E_AF6012_BAD_DECL_SYNTAX,0));
	}
}

/*
** Name:	_ScanName() -	Scan an Input String for a Type Name.
**
** Description:
**	Takes the referenced input string and parses the type name from it into
**	the output buffer.  The input string reference is updated to point to
**	the character following the type name in the string.
**
**	The type name will be lower-cased.
**
** Inputs:
**	instring	{char **}  A reference to the type specification string.
**
**	type_name	{char []}  Type name output buffer.
**
** Outputs:
**	instring	{char **}  A reference to the type specification string
**			just after the type name part of it.
**
**	type_name	{char []}  Result string holding the type name.
**
** History:
**	Written	2/6/87	(dpr)
*/
static VOID
_ScanName ( instring, type_name )
char	**instring;
char	*type_name;
{
	register char	*ip = *instring;
	register char	*op = type_name;
	bool		multiword = TRUE; /* for the "long varchar" case */

	/*
	** more debugging protocol?
	** SIprintf("in ScanName, ip = %s, instring = %s\n", ip, *instring);
	*/

	while ( CMwhite(ip) )	/* strip leading blanks */
	{
		CMnext(ip);
	}
	while ( multiword )
	{
		if ( CMnmstart(ip) )	/* grab the first char */
		{
			CMcpyinc(ip, op);
			while ( CMnmchar(ip) )	/* get the rest of the name */
			{
				CMcpyinc(ip, op);
			}
			while ( CMwhite(ip) )	/* strip trailing blanks */
			{
				CMnext(ip);
			}
		}
		else
		{
			multiword = FALSE;
		}
		/* this is probably inefficient to do this comparison twice */
		if ( CMnmstart(ip) )		/* another word, add a blank */
		{
			*op++ = ' ';		/* we know a blank is 1 byte */
		}
	} /* end while multiword */
	*op = EOS;
	CVlower(type_name);

	*instring = ip;
}

/*{
** Name:	iiafScanNum() -	Scan a Number in a Type Specification.
**
** Description:
**	Takes the input string, and converts the char
**	version of the number it was pointing to into numeric form.
**	the number is returned in num, and the input string is
**	updated.
**
** Inputs:
**	numstr		pointer to input string containing the length.
**
**	num		pointer to a i4  to hold the number
**
** Outputs:
**	num		the converted numeric value.
**
** Returns:
**	{DB_STATUS}	OK
**			status from 'CVan()'
**
** History:
**	Written	2/6/87	(dpr)
**	6/25/87 (danielt) removed unused variable from iiafScanNum()
**	07-aug-95 (albany)    #69997
**	    Put in a check so we don't write past the end of the
**	    number[] buffer.  Also supplied support for ports to
**	    platforms with multi-byte numbers.
*/
DB_STATUS
iiafScanNum ( numstr, num )
char 	**numstr;
i4	*num;
{
	char	*str;
	char	*nump;
	i4	rval;
	char	number[ SCAN_NUM_BUF_LEN + 1 ];
	i4	count = 0;

	str = *numstr;
	nump = number;
	*num = 0;
	while (CMwhite(str))	/* strip leading blanks */
	{
		CMnext(str);
	}
	while (CMdigit(str))
	{
	        /* 
		** we need to check here if we're exceeding
		** the buffer boundary; bad things happen
		** when we do...
		**
		** CV_SYNTAX seemed as reasonable an error
		** signal as any.
		**
		** See the comments at the head of the file
		** for setting up the SCAN_NUM_* defines to
		** handle multi-byte numbers.
		*/
	        if ( SCAN_NUM_LOOP_CNT < count++ )
		    return CV_SYNTAX;
		CMcpyinc(str,nump);	/* isn't a number always 1 byte ? */
	}

	*nump = EOS; 		/* null terminate the number */

	if ((rval = CVan(number,num)) != OK)
		return rval;

	while (CMwhite(str))	/* strip trailing blanks */
	{
		CMnext(str);
	}
	*numstr = str;

	/*
	**	debugging protocol ?
	SIprintf("iiafScanNum: returning %d\n", *num);
	*/

	return OK;
}


/*{
** Name:	IIAFfedatatype - Check if datatype is supported by FE tools.
**
** Description:
**	Given a DB_DATA_VALUE, this routine will let the user know if
**	the datatype specified in the DBV is currently supported in the
**	FE tools.
**
**	Datatype current supported are:
**	- DB_FLT_TYPE (floating point number)
**	- DB_CHR_TYPE (c)
**	- DB_TXT_TYPE (text)
**	- DB_DTE_TYPE (date)
**	- DB_MNY_TYPE (money)
**	- DB_DEC_TYPE (decimal)
**	- DB_CHA_TYPE (char)
**	- DB_VCH_TYPE (varchar)
**
** Inputs:
**	dbv		Pointer to DB_DATA_VALUE to check.
**			We really just check the db_datatype struct member.
**
** Outputs:
**
**	Returns:
**		TRUE	If the datatype is supported by the FE tools.
**		FALSE	If the datatype is not supported by the FE tools.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/01/92 (dkh) 
**	   Initial version.
**	07/09/06 (gupsh01)
**	   Added support for ANSI date/time datatypes.
*/
bool
IIAFfedatatype(DB_DATA_VALUE *dbv)
{
	bool	retval;

	switch(abs(dbv->db_datatype))
	{
	  case DB_INT_TYPE:
	  case DB_FLT_TYPE:
	  case DB_CHR_TYPE:
	  case DB_TXT_TYPE:
	  case DB_DTE_TYPE:
	  case DB_ADTE_TYPE:
	  case DB_TMWO_TYPE:
	  case DB_TMW_TYPE:
	  case DB_TME_TYPE:
	  case DB_TSWO_TYPE:
	  case DB_TSW_TYPE:
	  case DB_TSTMP_TYPE:
	  case DB_INYM_TYPE:
	  case DB_INDS_TYPE:
	  case DB_MNY_TYPE:
	  case DB_DEC_TYPE:
	  case DB_CHA_TYPE:
	  case DB_VCH_TYPE:
		retval = TRUE;
		break;

	  default:
		retval = FALSE;
		break;
	}

	return(retval);
}
