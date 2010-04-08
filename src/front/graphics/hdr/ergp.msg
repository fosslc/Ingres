/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ergp.msg -	Message file for class GP
**
** Description:
**	This file defines the messages for class GP.
**
** History:
**	 5-aug-1987 (bobm)
**		Created.
**/

# define	_GP_CLASS	69
/*
**	cpgraph.c
*/
/*
** status messages output during copying:
*/
F_GP0002_into_database	"Copying graph into database %0c from file %1c . . . "
F_GP0003_from_database	
"Copying graph %0c from database %1c into file %2c . . . "
F_GP0004_done	"done."
/*
** fatal error - not supposed to happen
*/
S_GP0001_Exception_set	"copygraph: could not set exception handler" /* DIAG */
/*
**	cpgraph.c
*/
S_GP0002_Exists_prompt	"Graph %0c already exists.  Do you wish to overwrite it?"
