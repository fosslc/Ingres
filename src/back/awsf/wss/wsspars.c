/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wsspars.h>
#include <erwsf.h>
#include <wsmvar.h>

/*
**
**  Name: wsspars.c - Parser to interpret HTML variable
**
**  Description:
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      11-Sep-1998 (fanra01)
**         Fixup build errors on unix.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  20-Aug-2003 (fanra01)
**      Bug 110747
**      Additional output information on encountering a parser error. 
**/

#define FIRST_VARIABLE_NODE 5

typedef struct __VAR_PARSER_OUT {
	ACT_PSESSION	act_session;
	char*	name;
	u_i4 namePos;
	char*	value;
	u_i4 valuePos;
	u_i4 HexProcessing;
	u_i4 type;
} VAR_PARSER_OUT, *PVAR_PARSER_OUT;

GSTATUS ParserGetVariable	(PPARSER_IN	in, PPARSER_OUT		out);
GSTATUS ParserGetName		(PPARSER_IN	in, PPARSER_OUT		out);
GSTATUS ParserReplaceBySpace(PPARSER_IN	in, PPARSER_OUT		out);
GSTATUS ParserOpenHex		(PPARSER_IN	in, PPARSER_OUT		out);
GSTATUS ParserAddHex		(PPARSER_IN	in, PPARSER_OUT		out);
GSTATUS ParseEmpty			(PPARSER_IN	in, PPARSER_OUT		out);

PARSER_NODE Variable[] = {
/* 0 */		{'0',		'9',		2,  5,	FALSE	, FALSE	, ParserAddHex			, NULL},
/* 1 */		{'A',		'F',		2,  5,	FALSE	, FALSE	, ParserAddHex			, NULL},
/* 2 */		{'=',		'=',		8,  11, FALSE	, FALSE	, ParserGetName			, ParserGetVariable},
/* 3 */		{'+',		'+',		2,  5,	FALSE	, FALSE	, ParserReplaceBySpace, NULL},
/* 4 */		{'%',		'%',		12, 14, FALSE	, FALSE	, ParserOpenHex			, NULL},
/* 5 */		{ (char)32, (char)127,  2,  5,	TRUE	, TRUE	, NULL			, ParseEmpty},

/* 6 */		{'0',		'9',		8,  11,	FALSE	, FALSE	, ParserAddHex			, ParserGetVariable},
/* 7 */		{'A',		'F',		8,  11,	FALSE	, FALSE	, ParserAddHex			, ParserGetVariable},
/* 8 */		{'&',		'&',		2,  5,	FALSE	, FALSE	, ParserGetVariable	, NULL},
/* 9 */		{'+',		'+',		8,  11,	FALSE	, FALSE	, ParserReplaceBySpace	, NULL},
/* 10 */	{'%',		'%',		15, 17, FALSE	, FALSE	, ParserOpenHex			, NULL},
/* 11 */	{ (char)32, (char)127,  8,  11,	TRUE	, TRUE	, NULL			, ParserGetVariable},

/* 12 */	{'0',		'9',		0,	5,	FALSE	, FALSE	, ParserAddHex			, NULL},
/* 13 */	{'A',		'F',		0,	5,	FALSE	, FALSE	, ParserAddHex			, NULL},
/* 14 */	{'%',		'%',		2,	5,	FALSE	, FALSE	, ParserAddHex			, NULL},

/* 15 */	{'0',		'9',		6,	11, FALSE	, FALSE	, ParserAddHex			, ParserGetVariable},
/* 16 */	{'A',		'F',		6,	11, FALSE	, FALSE	, ParserAddHex			, ParserGetVariable},
/* 17 */	{'%',		'%',		8,	11, FALSE	, FALSE	, ParserAddHex  	, ParserGetVariable},
};

/*
** Name: ParseEmpty() - Entry point
**
** Description:
**
** Inputs:
**	ACT_PSESSION	active session
**	char*			var
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
ParseEmpty(
	PPARSER_IN	in, 
	PPARSER_OUT		out)
{
return(GSTAT_OK);
}

/*
** Name: ParseHTMLVariables() - Entry point
**
** Description:
**
** Inputs:
**	ACT_PSESSION	active session
**	char*			var
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      11-Sep-1998 (fanra01)
**          Separate declaration and initialisation for building on unix.
**      20-Aug-2003 (fanra01)
**          Additional output information if parse error encountered.
*/
GSTATUS 
ParseHTMLVariables(
	ACT_PSESSION act_session, 
	char *var) 
{
	GSTATUS err = GSTAT_OK;	
	if (var != NULL) 
	{
		PARSER_IN in;
		VAR_PARSER_OUT out;

		MEfill (sizeof(PARSER_IN), 0, (PTR)&in);
		in.first_node = FIRST_VARIABLE_NODE;
		in.buffer = var;
		in.length = STlength(var);
		
		MEfill (sizeof(VAR_PARSER_OUT), 0, (PTR)&out);
		out.act_session = act_session;
		out.HexProcessing = FALSE;
		out.type = WSM_ACTIVE;
		
		err = ParseOpen(Variable, &in, (PPARSER_OUT) &out);
		if (err == GSTAT_OK)
			err = Parse(Variable, &in, (PPARSER_OUT) &out);
		if (err == GSTAT_OK)
			err = ParseClose(Variable, &in, (PPARSER_OUT) &out);
        if (err != GSTAT_OK)
        {
            DDFStatusInfo( err, "Variable name = %s\n", var );
        }
	}
return(err);
}


/*
** Name: ParserGetVariable() - 
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS ParserGetVariable (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	GSTATUS err = GSTAT_OK;	
	PVAR_PARSER_OUT pout;
	u_i4 length;
	char*	name = NULL;
	u_i4	nlen = 0;
	char*	value = NULL;
	u_i4	vlen = 0;
	char	nullval[] = "";

	pout = (PVAR_PARSER_OUT) out;

	if (pout->namePos == 0) 
	{
		err = DDFStatusAlloc(E_WS0012_WSS_VAR_BAD_NAME);
	} 
	else 
	{
		length = in->current - in->position;
    if (length > 0)
    {
      u_i4 size = ((((pout->valuePos + length) / 32) + 1) * 32);
      err = GAlloc(&pout->value, size, TRUE);
      if (err == GSTAT_OK)
      {
  		  MECOPY_VAR_MACRO(in->buffer + in->position, length, pout->value + pout->valuePos);
	  	  pout->valuePos += length;
      }
    }
		in->position = in->current+1;

		if (pout->namePos > 0 && pout->name[0] == VARIABLE_MARK)
			err = WSMGetVariable (
							pout->act_session, 
							pout->name + 1, 
							pout->namePos - 1, 
							&name, 
							WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

		if (err == GSTAT_OK && name == NULL)
		{	
			name = pout->name;
			nlen = pout->namePos;
		}
		else
			nlen = STlength(name);

		if (pout->valuePos <= 0)
		{
			vlen = 1;
			value = nullval;
		}
		else
		{
			if (pout->value[0] == VARIABLE_MARK)
				err = WSMGetVariable (
							pout->act_session, 
							pout->value+ 1, 
							pout->valuePos - 1, 
							&value, 
							WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

			if (err == GSTAT_OK && value == NULL)
			{
				value = pout->value;
				vlen = pout->valuePos;
			}
			else
				vlen = STlength(value);
		}
		err = WSMAddVariable (
					pout->act_session, 
					name, 
					nlen, 
					value, 
					vlen, 
					pout->type);
	}
  MEfree((PTR)pout->name);
	pout->name = NULL;
  MEfree((PTR)pout->value);
	pout->value = NULL;
	pout->namePos = 0;
	pout->valuePos = 0;
return(err); 
}

/*
** Name: ParserGetName() - 
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
ParserGetName (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	GSTATUS err = GSTAT_OK;	
	PVAR_PARSER_OUT pout;
	u_i4 length;
  u_i4 size;

	pout = (PVAR_PARSER_OUT) out;

	length = in->current - in->position;
  size = ((((pout->valuePos + length) / 32) + 1) * 32);
  err = GAlloc(&pout->name, size, FALSE);
  if (err == GSTAT_OK && pout->valuePos > 0)
  {
    MECOPY_VAR_MACRO(pout->value, pout->valuePos, pout->name);
    pout->namePos = pout->valuePos;
    pout->valuePos = 0;
  }

	MECOPY_VAR_MACRO(in->buffer + in->position, length, pout->name + pout->namePos);
	pout->namePos += length;

	in->position = in->current+1;
return(err); 
}

/*
** Name: ParserReplaceBySpace() - 
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
ParserReplaceBySpace (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	GSTATUS err = GSTAT_OK;	
	PVAR_PARSER_OUT pout;
	pout = (PVAR_PARSER_OUT) out;

	in->buffer[in->current] = SPACE;	
	pout->HexProcessing = FALSE;
return(err); 
}

/*
** Name: ParserOpenHex() - 
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
ParserOpenHex (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	GSTATUS err = GSTAT_OK;	
	PVAR_PARSER_OUT pout;
	u_i4 length;
  u_i4 size;
	pout = (PVAR_PARSER_OUT) out;

	length = in->current - in->position;
  size = ((((pout->valuePos + length) / 32) + 1) * 32);  
  err = GAlloc(&pout->value, size, TRUE);
  if (err == GSTAT_OK)
  {
	  MECOPY_VAR_MACRO(
			    in->buffer + in->position, 
			    length, 
			    pout->value + pout->valuePos);
	  pout->valuePos += length;
	  pout->HexProcessing = pout->valuePos++;
	  pout->value[pout->HexProcessing] = EOS;
  }
	in->position = in->current + 1;
return(err); 
}

/*
** Name: ParserAddHex() - 
**
** Description:
**
** Inputs:
**	PPARSER_IN		in 
**	PPARSER_OUT		out
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
ParserAddHex (
	PPARSER_IN		in, 
	PPARSER_OUT		out)
{
	GSTATUS err = GSTAT_OK;	
	PVAR_PARSER_OUT pout;
	pout = (PVAR_PARSER_OUT) out;

	if (in->buffer[in->current] == '%')
		pout->value[pout->HexProcessing] = '%';
	else
		pout->value[pout->HexProcessing] = 
					(char) AddHexa(	(u_i4)pout->value[pout->HexProcessing], 
													in->buffer[in->current]);
	in->position = in->current + 1;
return(err); 
}

