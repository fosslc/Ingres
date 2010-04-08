/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    oslkw.h -	OSL Parser Keyword Table Entry Definition.
**
** Description:
**	Contains the definition of the OSL parser keyword entry structure.
**
** History:
**	Revision 3.0
**		Initial revision.
**
**	Revision 6.0  87/06  wong
**		Added OSXCONST token type.
**
**	Revision 6.3/03/00  90/03  wong
**		Added token map types for BEGIN, END.
**	01-nov-89 (jfried)
**		Added token map type for "$ingres".
**
**	Revision 6.5
**	18-jun-92 (davel)
**		Added OSDCONST token type for decimal support.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

/*}
** Name:    KW -	Keyword Type Description.
*/
typedef struct _kwtab {
	const char	*kwname;
	i4		kwtok;
	bool		kwequel;	/* Whether it is an EQUEL keyword */
	const struct _kwtab  *kwdbltab;
} KW;

/*
** These constants are used by the scanners in the QUEL and SQL
** versions to access the token values for certain tokens the scanner
** has to know about.  They are used to access a table (osscankw) that maps
** from the constants to the actual token values.
*/

GLOBALCONSTREF i4	osscankw[];

#define	    OSIDENT		0
#define	    OSICONST		1
#define	    OSFCONST		2
#define	    OSSCONST		3
#define	    OSDEREFERENCE	4
#define	    OSCOLEQ		5
#define	    OSNOTEQ		6
#define	    OSLTE		7
#define	    OSGTE		8
#define	    OSEXP		9
#define	    OSCOLID		10
#define	    OSLSQBRK		11
#define	    OSRSQBRK		12
#define	    OSXCONST		13
#define	    OSINGRES		14
#define	    OSBEGIN		15
#define	    OSEND		16
#define	    OSDCONST		17

