/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	fglstr.h	- Linked string include
**
** Description:
**	Define structures and constants used by linked string routines
**
** History:
**	15 May 1989 (Mike S) -	Initial version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


# define LSTR_LENGTH	255

/*
**	Linked string member.
*/
struct _lstr
{
	char 		*string;		/* string text */
	struct _lstr 	*next;			/* Next string */
	i4		follow;			/* text to follow string */
# define LSTR_NONE	0		/* Nothing added */	
# define LSTR_AND	1		/* " AND"	*/
# define LSTR_COMMA	2		/* ","		*/
# define LSTR_SEMICOLON	3		/* "; " 	*/
# define LSTR_EQUALS	4		/* "=" 		*/
# define LSTR_OPEN	5		/* "(" 		*/
# define LSTR_CLOSE	6		/* ")" (code assumes this is last one)*/
	bool		nl;			/* mandatory newline */
	bool		sc;			/* terminate with semicolon */
	char		buffer[LSTR_LENGTH+1];  /* String buffer */
};

typedef struct _lstr	LSTR;
