/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    erfc.msg -	Form Compilation Message Definitions File.
**
** Description:
**	Contains message definitions for the Forms Compilation module
**	including the Forms Compilation program, CompForm.
**
** History:
**	Revision 6.0  87/08/14  dave
**	14-aug-1987 (dave)
**		Created.
**	09/30/87 (dkh) - Added message ids E_FC0004_M_NOT_SUPPORTED
**			 and S_FC0005_Written.
**	10/21/88 (dkh) - Fixed venus bug 3708.
**/

# define	_FC_CLASS	54
/*
**	compfrm.c
*/
E_FC0001_can_t_build_fr	"ERROR: can't build frame for %s\n" /* DIAG */

S_FC0002_Writing
"Writing compiled version of form \"%0c\" to file \"%1c\" . . ."
/*
**	getnum.c
*/
E_FC0003_COMPFRM__too_many_obj	"COMPFORM: too many objects to save" /* DIAG */

/*
**	compform.qc
*/
E_FC0004_M_NOT_SUPPORTED	"-m:  not supported in this environment.\n
The -m flag is supported only in the VAX/VMS environment."

/*%
** Explanation:
**	The "-m" flag (which specifies `macro' or assembler output) is supported
**	in the VAX/VMS environment only.  Compiled forms are always created as
**	C language structures in all environments other than VAX/VMS.
**
** System Status:
**	The program will ignore the "-m" flag.
**
** User Action:
**	Do not use the "-m" flag in subsequent calls to "compform".
*/
/*
**	compfrm.c
*/
S_FC0005_Written
"Compiled version of form \"%0c\" successfully written to file \"%1c\"."

/*%
** Explanation:
**	Status message indicating the compform ran successfully.
**
** System Status:
**
** User Action:
**	None.
*/

E_FC0006_bad_file
"Can not open file \"%0c\" for output.  Please check that you have
specified a legal filename and that you have permission to create
files in the desired directory."

/*%
** Explanation:
**	User specified a bad filename or did not have write
**	permission for the desired directory.
**
** System Status:
**	System will continue for vifred or exit for compform.
**
** User Action:
**	Clear message to continue processing.
*/
