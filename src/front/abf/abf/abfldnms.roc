/*
** Copyright (c) 1989, 2008 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>

/**
** Name:	abfldnms.roc -	ABF Component Object Edit Form Field Names.
**
** Description:
**	Contains the definitions of the component object edit form field names.
**
**	_srcfile	source-code file name field.
**	_return_type	return type field.
**	_data_type	data type field (for globals).
**	_yn_field_title		title for 'nullable' field for globals.
**	_nullable	return type nullable field.
**	_fstatic	static frame specification field.
**	_formname	form name field.
**	_outfile	subsystem output file name field.
**	_comline	subsystem command line field.
**	_symbol	
**	_library
**	_language
**	_warning
**	_type
**	_value		constant value
**
** History:
**	Revision 6.2  89/02  wong
**	Initial revision.
**
**	Revision 6.5
**	03-dec-92 (davel)
**		Added _passdec (for "passing decimal to 3GL" specification).
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

/*:
** Name:	Field Names
*/

GLOBALDEF const char	_srcfile[]	= ERx("sourcefile");
GLOBALDEF const char	_return_type[]	= ERx("return_type");
GLOBALDEF const char	_data_type[]	= ERx("data_type");
GLOBALDEF const char	_nullable[]	= ERx("nullable");
GLOBALDEF const char	_fstatic[]	= ERx("fstatic");
GLOBALDEF const char	_formname[]	= ERx("form");
GLOBALDEF const char	_outfile[]	= ERx("output");
GLOBALDEF const char	_comline[]	= ERx("comline");
GLOBALDEF const char	_symbol[]	= ERx("symbol");
GLOBALDEF const char	_library[]	= ERx("library");
GLOBALDEF const char	_language[]	= ERx("language");
GLOBALDEF const char	_warning[]	= ERx("warning");
GLOBALDEF const char	_yn_field_title[]	= ERx("yn_field_title");
GLOBALDEF const char	_type[]		= ERx("type");
GLOBALDEF const char	_value[]	= ERx("value");
GLOBALDEF const char	_passdec[]	= ERx("pass_dec");

