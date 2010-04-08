/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ercg.msg -	Code Generator Error Message Definitions File.
**
** Description:
**	Contains the error message definitions for the code generator.
**	Since the code generator uses as input the intermediate language
**	created by the OSL translator, it should never encounter an
**	error in its input.  An error message from the code generator
**	indicates a bug that should be reported to RTI.
**
** History:
**	Revision 6.0  87/08/07  arthur
**	Initial revision.
**	16-mar-94 (lan)
**		Fixed bug #58066 - Replaced 'Technical Support Representative'
**		with 'Site Technical Contact'.
**/

#define	_CG_CLASS	170

E_CG0000_noout	"Could not open output file for write.\n
The OSL code generator could not open or write to the output file `%0c' (in the
ABF object code directory.)  Please check that you have access rights to the
directory and the file if it exists.  Also, check that you have enough disk
space to create files within this directory."

/*%
** Explanation:
**	The code generator could not open the file specified on the command-line
**	(passed by ABF) into which it will generate C code for a frame/procedure
**	object.  This could be for several reasons, but most likely is because
**	you do not have access rights either to the directory in which the file
**	is to be created (usually the ABF object code directory) or to the
**	output file if it already exists, or because you have exceeded the disk
**	space available to you in that directory.
**
** System Status:
**	The code generator will terminate immediately with a FAIL status.  No
**	object file will be produced for this frame/procedure.
**
** User Action:
**	Make sure that the you have both access rights to the ABF object code
**	directory (or output file if it exists) and that enough disk space is
**	available to you there and then retry the operation before reporting
**	this as a problem to your Site Technical Contact.
*/

E_CG0001_sessopen
"An error occurred in opening a session of the intermediate language
retrieval facility.\ 
Please report this internal problem to your Site Technical Contact."

/*%
** Explanation:
**	DIAGNOSTIC.
**
** System Status:
**	The code generator will return a status of FAIL.  No object
**	file will be produced for this frame/procedure.
**
** User Action:
**	Report this problem to your Site Technical Contact.
*/

E_CG0002_label
"An error occurred in the Label Module of the OSL code generator
during `%0c'.\ 
Please report this internal problem to your Site Technical Contact."

/*%
** Explanation:
**	DIAGNOSTIC.  An internal coding error occurred in the routines within
**	the code generator that handle the generation of labels in the C
**	language output file.  This is an internal program error.
**
** System Status:
**	The code generator will return a status of FAIL.  No object
**	file will be produced for this frame/procedure.
**
** User Action:
**	Report this problem to your Site Technical Contact.
*/

E_CG0003_comline
"The command line for the 4GL code generator could not be
parsed, or contained too many or too few arguments.\ 
Please report this internal problem to your Site Technical Contact."

/*%
** Explanation:
**	DIAGNOSTIC.  This error appears when the code generator receives the
**	wrong number of arguments on the command line.  Since the command line
**	is built by ABF, this error should never occur, and is an internal
**	program error.
**
** System Status:
**	The code generator will return a status of FAIL.  No object
**	file will be produced for this frame/procedure.
**
** User Action:
**	Report this problem to your Site Technical Contact.
*/

E_CG0004_objbad
"An error occurred in retrieving information about the frame or
procedure `%0c' from the object manager catalogs.\ 
Please report this internal problem to your Site Technical Contact."

/*%
** Explanation:
**	DIAGNOSTIC.  The code generator could not retrieve the intermediate
**	frame object for a frame or procedure from the internal facility ILRF.
**	This is an internal program error.
**
** System Status:
**	The code generator will return a status of FAIL.  No object
**	file will be produced for this frame/procedure.
**
** User Action:
**	Report this problem to your Site Technical Contact.
*/

E_CG0005_eofil
"An unexpected end-of-file was reached in the intermediate language.\ 
Please report this internal problem to your Site Technical Contact."

/*%
** Explanation:
**	DIAGNOSTIC.  The code generator reached a NULL operator when reading the
**	intermediate language (IL) for a frame or procedure.  This might show
**	that the IL terminated prematurely because of some error in writing or
**	reading the IL, or that the code generator had an internal program error
**	in moving to the next IL statement to be read.
**
** System Status:
**	The code generator will return a status of FAIL.  No object
**	file will be produced for this frame/procedure.
**
** User Action:
**	Report this problem to your Site Technical Contact.
*/

E_CG0006_ilmissing
"An intermediate language statement with operator `%0c' was expected,
but not found.\ 
Please report this internal problem to your Site Technical Contact."

/*%
** Explanation:
**	DIAGNOSTIC.  The code generator expected, but did not find, an
**	intermediate language (IL) statement beginning with the specified
**	operator.  The code generator was in the middle of reading IL for an
**	4GL statement that translates into more than one IL statement.  This
**	is an internal program error.
**
** System Status:
**	The code generator will return a status of FAIL.  No object
**	file will be produced for this frame/procedure.
**
** User Action:
**	Report this problem to your Site Technical Contact.
*/
