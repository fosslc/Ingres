/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    ertl.msg -	Tables Utility Message Definitions File.
**
** Description:
**	Contains the definitions for the messages used by the Tables utility.
**
** History:
**	Revision 6.0  87/08/11  daver
**		Created.
**/

# define	_TL_CLASS	178
/*
**	tables.qc
*/
E_TL0001_TestingError	"Internal program error:  Testing initialization error\n
Please report this internal problem to your technical representative."

/*%
** Explanation:
**	DIAGNOSTIC.
**
** System Status:
**	The Tables Utility will terminate.
**
** User Action:
**	Report this internal problem to your technical representative.
*/

E_TL0002_BadTempDir	"Cannot create temporary files.\n
The temporary files directory (specified by II_TEMPORARY) or the current working
directory cannot have files created in it.  Please check that you have have
access rights to the directory and that sufficient disk space exists to create
and write files there."

/*%
** Explanation:
**	The directory in which this program expects to be able to create and
**	write temporary files cannot have files created within it.  This
**	directory is either the temporary files directory specified by the
**	II_TEMPORARY environment variable, or is the current working
**	directory.  The likely reason for this is that you do not have
**	access rights to create files within this directory, or that
**	insufficient disk space exists in this directory for you to create 
**	or write files within it.
**
** System Status:
**	The Tables Utility will terminate.
**
** User Action:
**	Make sure that you both have access rights to the temporary directory
**	and that sufficient disk space is available to you there.  Then retry
**	execution of the Tables Utility.
*/

E_TL0003_BadSetup	"Internal program error: could not set exception handler\n
Please report this internal problem to your technical representative."

/*%
** Explanation:
**	DIAGNOSTIC.
**
** System Status:
**	The Tables Utility will terminate.
**
** User Action:
**	Report this internal problem to your technical representative.
*/
