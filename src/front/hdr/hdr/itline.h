/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ITline.h	- header file for drawing line functions
**
** Description:
**	This file contains the definitions for the drawing line
**
** History:
**	06-nov-1986 (yamamoto)
**		first written
**
**	15-nov-91 (leighb) DeskTop Porting Change:
**		Changed internal codes of the line drawing characters so
**		that they do not conflict with the section marker (^U) on
**		the IBM/PC.
**	11-nov-2009 (wanfr01) b122875
**	    Moved constants from itdrloc.h since they are normally used
**	    together.
**/

/*
**	internal character codes of the drawing line
*/

# define DRCH_LR	'\016'		/* lower-right corner of a box	*/	 
# define DRCH_UR	'\017'		/* upper-right corner of a box	*/	 
# define DRCH_UL	'\020'		/* upper-left corner of a box	*/	 
# define DRCH_LL	'\021'		/* lower-left corner of a box	*/	 
# define DRCH_X		'\022'		/* crossing lines		*/	 
# define DRCH_H		'\023'		/* horizontall line		*/	 
# define DRCH_LT	'\024'		/* left		'T'		*/	 
# define DRCH_SEC	'\025'		/* Section Mark Character	*/	 
# define DRCH_RT	'\026'		/* right	'T'		*/	 
# define DRCH_BT	'\027'		/* bottom	'T'		*/	 
# define DRCH_TT	'\030'		/* top		'T'		*/	 
# define DRCH_V         '\031'		/* vertical bar                 */	 


/*
**	declare functions that return characer pointer.
*/

FUNC_EXTERN char	*ITldopen();
FUNC_EXTERN char	*ITldclose();


/*
**      definition of the drawing character table.
*/

typedef struct
{
        u_char  *drchar;        /* pointer to the drawing characters */
        i4      drlen;          /* length of the string 'drchar' */
        u_char  *prchar;        /* pointer to the characters that
                                ** are consisted by ASCII char
                                */
        i4      prlen;          /* length of the string 'prchar' */
} DRLINECHAR;

GLOBALREF DRLINECHAR    drchtbl[];


/*
**      index of drchtbl[]
*/

# define DRLD   0       /* init terminal to drawing lines               */
# define DRLE   1       /* interpret subseq. chars as regular chars     */
# define DRLS   2       /* interpret subseq. chars for drawing chars    */
# define DRQA   3       /* lower-right corner of a box  */
# define DRQB   4       /* upper-right corner of a box  */
# define DRQC   5       /* upper-left corner of a box   */
# define DRQD   6       /* lower-left corner of a box   */
# define DRQE   7       /* corssing lines               */
# define DRQF   8       /* horizontal line              */
# define DRQG   9       /* left         'T'             */
# define DRQH   10      /* right        'T'             */
# define DRQI   11      /* bottom       'T'             */
# define DRQJ   12      /* top          'T'             */
# define DRQK   13      /* vertical bar                 */

# define NDRINX 14      /* number of DR index   */


