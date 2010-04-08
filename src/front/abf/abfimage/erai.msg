/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erai.msg -	Message file for class AI
**
** Description:
**	This file defines the messages for class AI.
**
** History:
**	 3-aug-1987 (bobm)
**		Created.
**	13-sep-1991 (blaise)
**		Changed abfimage (the old name for the command) to imageapp
**		(the new name) throughout. Bug #39592.
**/

# define	_AI_CLASS	174
/*
**	abimain.c
*/
F_AI0001_EXE_NAME	"APPLICATION/IMAGE"

/*
** The argument is the file containing the the application image.
** It will either be a default name that imageapp created, or one
** that the user gave.
*/
S_AI0001_EXE_MADE	"The application image %0c was successfully built."

/*
** The argument is the file containing the the application image.
** It will either be a default name that imageapp created, or one
** that the user gave.
*/
E_AI0002_EXE_NOT_MADE	"The application image was NOT built.\n
An earlier error prevents the application image from being built.
Correct the cause of the earlier error and then run imageapp again."
/*%
** Explanation:
**	Imageapp could not build an executable image of the application.
** System Status:
**	Imageapp will exit with a failure status.
** User Action:
**	An earlier error should have been printed.  By fixing the
**	earlier error, then running imageapp again, an executable image
**	should be successfully built.
*/
