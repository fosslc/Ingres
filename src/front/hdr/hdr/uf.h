/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#ifndef UF_H_INCLUDED
#define UF_H_INCLUDED

# include	<lo.h>

/**
** Name:	uf.h -	Front-End FRS Utilities Interface Defintions File.
**
** Description:
**	Contains values needed for UF module.
**
** History:
**	Revision 6.4  90/01  wong
**	Added UF_HELP structure.  90/05  Also FLD_VALUE structure.
**
**	Reviision 6.0  1987/06/15  joe
**	15-jun-1987 (Joe)
**		Created to support IIUFrtmRestoreTerminalMode
**	03-aug-1987 (peter)
**		Add declarations.
**      20-jun-2001 (bespi01)
**              Added constant for the default block size of a random
**              access file in VMS. This is used when opening the
**              spill file and associated other cruft.
**              Bug 104175 Problem INGCBT 335
**      07-mar-2002 (horda03)
**              Increased RACC_BLKSIZE from 512 to 32000. The previous
**              record size for the spill file caused performance
**              issues on VMS; now that the spill file is a RACC, there
**              appears to be a big overhead on VMS when writing/reading
**              records for a RACC file.
**              Bug 107271
**      04-Oct-2004 (hweho01)
**              Corrected the prototype declaration for IIUFgtfGetForm().
**	19-Aug-2009 (kschendel) 121804
**	    Allow repeated inclusion (e.g. by runtime.h)
**/

#define RACC_BLKSIZE (32000)

VOID		IIUFask();
STATUS          IIUFgtfGetForm( LOCATION * , char *  );
LOCATION	*IIUFlcfLocateForm();
VOID		IIUFrtmRestoreTerminalMode();
bool		IIUFver();
VOID		IIUFfsh_Flush();
VOID		IIUFfbi_FreeBrowseIdx();
VOID		iiufBrExit();
bool		IIUFmro_MoreOutput();
STATUS		IIUFgnr_getnextrcb();

/*
** Following constants are used by IIUFrtmRestoreTerminalMode
*/
# define	IIUFNORMAL	1
# define	IIUFFORMS	2
# define	IIUFPROMPT	3
# define	IIUFMORE	4

/*}
** Name:	UF_HELP -	Help File Strucutre.
**
** Description:
**	Contains the information necessary to display help using the Front-End
**	help utilities.  This includes the title as a message ID and the file
**	name, which is limited to eight characters (which is the coding standard
**	portability limit) plus the extension of ".hlp".
**
** History:
**	01/90 (jhw) -- Written.
*/
#define	HLP_FILE_NAME_SIZE	(8 + 1 + 3)	/* 8 + "." + "hlp" */

typedef struct {
	ER_MSGID	title;
	char		file[HLP_FILE_NAME_SIZE+1];
} UF_HELP;

/*}
** Name:	FLD_VALUE -	Field/Column Value Descriptor.
**
** Description:
**	Contains the information necessary to access a field or column value,
**	which is the form name, the field (or table field) name, and the column
**	name.
**
** History:
**	05/90 (jhw) -- Written.
*/
typedef struct {
	char	*_form;
	char	*_field;
	char	*_column;
} FLD_VALUE;

#endif
