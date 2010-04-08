/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
** eqlang.h
**	- definition of the language constants.
**
** History:
**	28-mar-1994 (teresal)	Add constant for the new ESQL/C++ 
**				preprocessor. Currently, 4GL cannot call
**				C++, so there is no OL support for C++. This
**				support might be added in the future, but
**				for now, assign EQ_CPLUSPLUS to the next 
**				number in the OLxxx list, i.e., 9 - look at
**				olcl.h. Note: a hostCPLUSPLUS define is not
**				required at this time; at runtime, Ingres
**				just sees C++ as C code.
**	12-feb-2003 (abbjo03)
**	    Rmove PL/1 as a supported language.
*/
# include	<ol.h>

# define EQ_C		OLC
# define EQ_FORTRAN	OLFORTRAN
# define EQ_PASCAL	OLPASCAL
# define EQ_BASIC	OLBASIC
# define EQ_COBOL	OLCOBOL
# define EQ_ADA		OLADA
# define EQ_OSL		OLOSL
# define EQ_CPLUSPLUS	9

/* Host definitions */
# define hostC			EQ_C
# define hostFORTRAN		EQ_FORTRAN
# define hostPASCAL		EQ_PASCAL
# define hostBASIC		EQ_BASIC
# define hostCOBOL		EQ_COBOL
# define hostADA		EQ_ADA
# define hostOSL		EQ_OSL

# define hostDML		(EQ_OSL+1)
