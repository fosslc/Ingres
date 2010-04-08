/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	abfgolnk.h -	ABF Test Mode Link Structure Definition.
**
** Description:
**	Contains the definition of the ABF test mode link object.  Defines:
**
**	ABLNKTST	test mode image description.
**
** History:
**	Revision 6.2  89/01  wong
**	Moved from old "abf!hdr!abfcat.h."
**
**	Revision 6.1  88/01  wong
**	Added test mode run options.
**
**	Revision 3.0  86/09/01  joe
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*}
** Name:	ABLNKTST -	The Run (Test Mode) Image Description.
**
** Description:
**	Describes the test mode image linked for the application (the test mode
**	is when GO is selected from ABF.)
**
**	For the DY case, the information is the actual addresses of
**	the tables for the runtime system.
**	
**	For the NODY case, the information needed is the name of the executable
**	that was built.  It gets run as a subprocess.
**
**	For the INTERP case, the information is the name of the file containing
**	the DS-encoded runtime tables.
**
** History:
**	1-sep-1986 (Joe)
**		First Written
**	01/88 (jhw) -- Added 'abrunopt' and made NODY always present if not
**		INTERP.
**	1/89 (bobm) - redefined.
**	08/89 (jhw) -- Added 'user_link' for when user libraries are linked
**		(only then will the interpreter be deleted on exit.)
**		JupBug #7357.
**	18-jun-92 (leighb) DeskTop Porting Change:
**		added 'abintimage' to point to the name to use when building
**		an interpreted image.
*/
typedef struct
{
	char	*entry_frame;
	bool	failed_comps;
	bool	redo_link;
	bool	plus_tree;
	bool	user_link;

	APPL_COMP	*one_frame;
	ABRTSOBJ	*abrunobj;	/* the runtime object */

	i4	hl_count;
	char	abintexe[MAX_LOC+1];	/* name of interpreter executable */
	char	abintmain[MAX_LOC+1];	/* name of interpreter main */
	char	*abintimage;		/* name of interpreted image to build*/		 
} ABLNKTST;
