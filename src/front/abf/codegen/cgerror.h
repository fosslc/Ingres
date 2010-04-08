/*
** Copyright (c) 2004 Ingres Corporation 
** All rights reserved.
*/


#include	"ercg.h"


/**
** Name:	CGERROR.h	-	Header file for error numbers
**
** Description:
**	Header file for error numbers for the code generator.
**
*/

/*
** Error numbers
*/
#define CG_NOOUT	E_CG0000_noout		/* Could not open output file
						   (filename) */
#define CG_SESSOPEN	E_CG0001_sessopen	/* Could not open ILRF session*/
#define CG_LABEL	E_CG0002_label		/* Error in label-generating
						   routines (type of error) */
#define CG_COMLINE	E_CG0003_comline	/* Wrong number of command
						   line args */
#define CG_OBJBAD	E_CG0004_objbad		/* Error in retrieving from obj
						   mgr catalogs (frame name) */
#define CG_EOFIL	E_CG0005_eofil		/* Unexpected EOF reached in IL
						   array */
#define CG_ILMISSING	E_CG0006_ilmissing	/* Expected IL statement not
						   found (operator expected) */
