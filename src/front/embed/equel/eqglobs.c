/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<si.h>
# include	<equel.h>

/*
+* Filename:	eqglobs.c
** Purpose:	Declare global variables for preprocessor.
**
** Defines Data:
**	_eq		- Global eq_state variable.
**	eq		- Global pointer to eq_state variable.
**	_dml		- Global dml_state variable.
**	dml		- Global pointer to dml_state variable.
**	sc_hostvar	- Global flag; when set supresses keyword lookups in
**			  sc_word.
**	yyreswd		- Global flag; when set enables parser reserved word
**			  retry
**
** History:
**	08-sep-1987 (ncg)
**	    Extracted from eqmain.c
**	27-jul-1992 (larrym)
**	    added global sc_hostvar
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-oct-2001 (somsa01)
**	    Moved definition of yyreswd to here.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPEMBEDLIBDATA
**	    in the Jamfile. Added LIBOBJECT hint which creates a rule
**	    that copies specified object file to II_SYSTEM/ingres/lib
*/

/*
**
LIBRARY = IMPEMBEDLIBDATA
**
LIBOBJECT = eqglobs.c
**
*/

GLOBALDEF EQ_STATE
		_eq ZERO_FILL,
		*eq = &_eq;
GLOBALDEF DML_STATE
		_dml ZERO_FILL,
		*dml = &_dml;
GLOBALDEF i4	sc_hostvar = FALSE;
GLOBALDEF bool  yyreswd = FALSE;
