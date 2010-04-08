/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>
#include	<cm.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>
# include       <er.h>

/**
** Name:	aftyhack.c -	Map Accepted Type Synonyms to ADF Types.
**
** Description:
**	ADF does not support all of the variants of all the various types,
**	and so, this file defines:
**
**	iiafTypeHack()	ad-hoc map of all accepted type synonyms.
**
** History:
**	Revision 6.2  89/05  wong
**	Added string type synonyms.  JupBug #6165.
**
**	Revision 6.0  87/05/01  daver
**	Initial revision.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Name:	SYNONYMS - 	Variant ADF Type Associations Map.
**
** Description:
**	The SYNONYMS table contains a mapping between the variants of the ADF
**	types that are accepted and the ADF types.  Most types are recognized
**	via ADF.  However, certain types such as some integer and float types
**	can have SQL sounding names which aren't currently recognized by ADF.
**	The "c" synonym is needed because it can contain a length immediately
**	following the type name "c" (e.g., c10) while the other string synonyms
**	stand for the string type with a length of 1.
**
**	NOTE: the entries in the SYNONYMS table MUST be arranged ALPHABETICALLY
**	      on synonym name in order for the binary search in iiafTypeHack()
**	      to work.
**
** History:
**	1-may-1987	(daver)	First Written.
**	01/04/89 (dkh) - Removed READONLY qualification to regain sharability
**				of ii_libqlib on VMS.
**	05/89 (jhw) - Added string type synonyms.  JupBug #6165.  Also,
**			ordered for binary search.
**	07-jun-1989 (wolf)
**		Synonyms on IBM must be in EBCDIC sort order for binary search.
**	20-aug-92 (davel)
**		Changed length argument to be a DB_DATA_VALUE pointer,
**		so that precision/scale could be hacked as well as length.
**		Also added definitions in the
**		synonym list for "dec", "decimal", and "numeric".
**	25-feb-93 (mohan)
**		Added tag to SYNONYMS structure.
**	12-mar-92 (fraser)
**		Changed structure tag name so that it doesn't begin with
**		underscore.
**	02-nov-1993 (mgw) Bug #55985
**		Added byte and byte varying to the synonyms table so that
**		if user enters either type without a length, it will default
**		to 1. Also added comment above about the need to keep the
**		SYNONYMS table alphabetical on synonym name in order for the
**		binary search in iiafTypeHack() to work.
**	05-May-2004 (hanje04)
**		SIR 111507 - BIGINT Support
**		To support use of 64bit tids and other BIGINTs define 8 byte 
**		integers (i8 & integer8) to synonyms table.
*/
typedef struct s_SYNONYMS
{
	char	*synonym;
	i4	len;
	i2	ps;
	char	adf_type[2];
} SYNONYMS;

static SYNONYMS	synonyms[] = {
	{"byte",	1, 0,	""},
	{"byte varying",	1, 0,	""},
	{"c",		1, 0,	""},
	{"char",	1, 0,	""},
	{"dec",		DB_PREC_TO_LEN_MACRO(5), DB_PS_ENCODE_MACRO(5,0), ""},
	{"decimal",	DB_PREC_TO_LEN_MACRO(5), DB_PS_ENCODE_MACRO(5,0), ""},
#ifndef IBM370
	{"f4",		4, 0,	"f"},
	{"f8",		8, 0,	"f"},
#endif
	{"float",	8, 0,	"f"},
	{"float4",	4, 0,	"f"},
	{"float8",	8, 0,	"f"},
#ifdef IBM370
	{"f4",		4, 0,	"f"},
	{"f8",		8, 0,	"f"},
#endif
#ifndef IBM370
	{"i1",		1, 0,	"i"},
	{"i2",		2, 0,	"i"},
	{"i4",		4, 0,	"i"},
	{"i8",		8, 0,	"i"},
#endif
	{"int",		4, 0,	"i"},
	{"integer",	4, 0,	"i"},
	{"integer1",	1, 0,	"i"},
	{"integer2",	2, 0,	"i"},
	{"integer4",	4, 0,	"i"},
	{"integer8",	8, 0,	"i"},
#ifdef IBM370
	{"i1",		1, 0,	"i"},
	{"i2",		2, 0,	"i"},
	{"i4",		4, 0,	"i"},
	{"i8",		8, 0,	"i"},
#endif
	{"numeric",	DB_PREC_TO_LEN_MACRO(5), DB_PS_ENCODE_MACRO(5,0), ""},
	{"smallint",	2, 0,	"i"},
	{"text",	1, 0,	""},
	{"varchar",	1, 0,	""}
};

#define SYNONYMS_SIZE	(sizeof(synonyms)/sizeof(SYNONYMS))

/*{
** Name:	iiafTypeHack() -	Ad-Hoc Mapping of Accepted Type Variants
**
** Description:
**	Maps all accepted type variants to their legal ADF types.  The type
**	specification is assumed to be lower-cased.
**
** Input params:
**	cb		{ADF_CB *}  The ADF control block.
**	typename	{char *}  The (lower-cased) type name in question.
**
** Output params:
**	cb		possible error info contained here.
**	
**	typename	type name, changed if necessary.
**
**	dbv->db_length	length of the passed type. 
**	dbv->db_prec	precision and scale of the passed type.
**
** Returns:
**	OK		Everything is peachy.
**
**	Anything else and the ADF_CB should be checked.
**
** History:
**	1-may-1987	(daver)	First Written.
**	05/89 (jhw) -- Modified to perform binary search.
*/
STATUS
iiafTypeHack ( cb, typename, dbv )
ADF_CB	*cb;
char	*typename;
DB_DATA_VALUE	*dbv;
{

	if ( *typename == 'c' && *(typename+1) != 'h' )
	{ /* check for special 'c[0-9]*' */
		if ( *++typename == EOS )
			dbv->db_length = 1;	/* accept 'c' with no length */
		else if (CMdigit(typename))
		{
			i4 len;
			if ( CVan(typename, &len) != OK )
				return afe_error(cb, E_AF6013_BAD_NUMBER, 0);
			*typename = EOS;
			dbv->db_length = len;
		}
	}
	else
	{
		register SYNONYMS	*syn;
		register SYNONYMS	*start;
		register SYNONYMS	*end;

		/* Binary search */
		start = synonyms;
		end = start + (SYNONYMS_SIZE - 1);
		do
		{
			register i4	dir;

			syn = start + (end - start) / 2;
			if ( (dir = *typename - *syn->synonym) == 0 &&
				(dir = STcompare(typename, syn->synonym)) == 0 )
			{ /* mapped it */
				dbv->db_length = syn->len;
				dbv->db_prec = syn->ps;
				STcopy( *syn->adf_type == EOS
						? syn->synonym : syn->adf_type,
					typename
				);
				break;
			}
			else if (dir < 0)
				end = syn - 1;
			else
				start = syn + 1;
		} while ( start <= end );
	}
	return OK;
}
