/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<st.h>
# include	<ug.h>
# include	<adf.h>
# include	<afe.h>
# include	<er.h>


/**
** Name:	vqpatmat.c - pattern matching support
**
** Description:
**	This file defines:
**		IIVQpePatternEncode	- encode pattern string
**		IIVQpePatternMatch	- attempt to match previous encoding
**
** History:
**	10/01/89 (tom)	- created
**      12-oct-93 (daver)
**              Casted parameters to STcat() to avoid compiler warnings
**              when this CL routine became prototyped.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      21-oct-2009 (joea)
**          Change matched to i1 to match DB_BOO_TYPE usage.
**/

static ADF_CB 		*cb; 
static DB_DATA_VALUE	dbv;
static DB_DATA_VALUE	*oparr[2];
static AFE_OPERS	oplist;
static ADI_FI_ID	func;
static DB_DATA_VALUE	result;
static i1		matched;


FUNC_EXTERN ADF_CB *FEadfcb();
FUNC_EXTERN STATUS IIAFcvWildCard();
FUNC_EXTERN STATUS IIAFpmEncode();
FUNC_EXTERN STATUS afe_opid();
FUNC_EXTERN STATUS afe_fdesc();


/*{
** Name:	IIVQpePatternEncode	- encode pattern matching characters
**
** Description:
**	Taking the string as input we will encode the pattern match characters
**	and setup the appropriate db data value structures, afe operation
**	lists etc..
**
** Inputs:
**	char *buf;	- char string to encode, assumed to be of maximum
**			  possible length FE_PROMPTSIZE
**	DB_TEXT_STRING *dbt - data string work space to use, must be at least
**			FE_PROMPTSIZE + 1 in size
**			NOTE: this buffer is assumed to be active and
**			unchanged by the caller during all calls to the 
**			PatternMatch function.
** Outputs:
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	10/21/89 (tom) - created
**      12-oct-93 (daver)
**              Casted parameters to STcat() to avoid compiler warnings
**              when this CL routine became prototyped.
*/
STATUS
IIVQpePatternEncode(buf, dbt)
char *buf;
DB_TEXT_STRING *dbt;
{
	ADI_OP_ID	eqopid;
	ADI_FI_DESC	fdesc;
	i4 		pm;	

	dbv.db_datatype = DB_TXT_TYPE;
	dbv.db_length = FE_PROMPTSIZE + sizeof(dbt->db_t_count);
	dbv.db_data = (PTR)dbt;

	dbt->db_t_count = STlength(buf);
	STcopy(buf, dbt->db_t_text);

	/* open a adf control block */
	cb = FEadfcb();

	/* convert wild char chars to quel usage */
	_VOID_ IIAFcvWildCard(&dbv, DB_QUEL, AFE_PM_CHECK, &pm);

	/* encode the pattern match characters */
	if (IIAFpmEncode(&dbv, TRUE, &pm) != OK)
	{
		FEafeerr(cb);
		return (FAIL);
	}

	/* if pattern match characters not found then we concatenate
	   on wild card characters on both ends so as to do an unanchored
	   search, this means we change our input string and 
	   encode it */
	if (pm == AFE_PM_NOT_FOUND)
	{
		dbt->db_t_text[0] = '*';
		STcopy(buf, &dbt->db_t_text[1]);
		STcat((char *)dbt->db_t_text, (char *)ERx("*"));
		dbt->db_t_count = STlength(buf) + 2;
		_VOID_ IIAFcvWildCard(&dbv, DB_QUEL, AFE_PM_CHECK, &pm);

		if (IIAFpmEncode(&dbv, TRUE, &pm) != OK)
		{
			FEafeerr(cb);
			return (FAIL);
		}

	}

	oplist.afe_ops = oparr;
	oplist.afe_opcnt = 2;
	oparr[0] = &dbv;
	oparr[1] = &dbv;

	result.db_datatype = DB_BOO_TYPE;
	result.db_length = sizeof(matched);
	result.db_data = (PTR)&matched;


	if (  afe_opid(cb, ERx("="), &eqopid) 
		!= OK
	   || afe_fdesc(cb, eqopid, &oplist, (AFE_OPERS*)NULL, &result, &fdesc)
		!= OK 
	   )
	{
		return (FAIL);
	}

	/* save the function instance id for when we do the comparison */
	func = fdesc.adi_finstid;

	return (OK);
}


/*{
** Name:	IIVQpmPatternMatch	- test for match against encoded pattern
**
** Description:
**	This function tests for a match between the input text and the
**	previously encoded pattern.
**
** Inputs:
**	char *buf;	- char string to test, must not be longer than
**			  FE_PROMPTSIZE, or we will return FALSE
**
** Outputs:
**	Returns:
**		TRUE if the caller's string is a match else FALSE
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	10/21/89 (tom) - created
*/
bool
IIVQpmPatternMatch(buf)
char *buf;
{
	AFE_DCL_TXT_MACRO(FE_PROMPTSIZE + 1) sbuf;
	DB_TEXT_STRING *str;
	DB_DATA_VALUE test;

	str = (DB_TEXT_STRING*)&sbuf;
	test.db_datatype = DB_TXT_TYPE;
	test.db_length = FE_PROMPTSIZE + sizeof(str->db_t_count);
	test.db_data = (PTR)str;	

	if ((str->db_t_count = STlength(buf)) > FE_PROMPTSIZE)
	{
		return (FALSE);
	}

	STcopy(buf, str->db_t_text);

	oparr[1] = &test;

	if (afe_clinstd(cb, func, &oplist, &result) != OK)
	{
		FEafeerr(cb);
	}

	return (bool)matched;
}
