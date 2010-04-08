/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	errc.msg -	Message file for class RC
**
** Description:
**	This file defines the messages for class RC.
**
** History:
**	31-jul-1987 (grant)
**		Created.
**	13-sep-1993 (rdrane)
**		Add level comment message (F_RC0003_Level_comment).
**/

# define	_RC_CLASS	92
/*
**	crcrack.c
*/
/*
**	Prompt for database name missing from command line invocation.
*/
S_RC0001_Database	"Database"
/*
**	Prompt for file name missing from command line invocation.
*/
S_RC0002_Filename	"Filename"
/*
**	Prompt for report name(s) missing from command line invocation.
**	More than one report name may be specified so it continues to
**	prompt until an empty line is entered (just a RETURN key).
**	
*/
S_RC0003_Report_name	"Report name (RETURN to end list)"
/*
**	crdmprep.c
*/
/*
**	Status message informing the user that the report specification is
**	being written to the file.
*/
S_RC0004_Writing_report	"Writing report '%s' specs to file '%s'.\n"
/*
**	crwrtrep.c
*/
/*
**	This is a comment put in the file giving the report name.
*/
F_RC0001_Report_name_comment	"**\tReport: %s"
/*
**	This is a comment put in the file giving the date that the report
**	was last modified.
*/
F_RC0002_Last_Modified_comment	"**\t\tLast Modified: %s"
/*
**	This is a comment put in the file giving the level of the report.
*/
F_RC0003_Level_comment	"**\t\tLevel: %d"
