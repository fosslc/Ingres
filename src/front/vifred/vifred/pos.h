/*
** pos.h
** structure definition for the position structure
** stores the start and end x and y for a feature
** name and pointer to the actual feature are also stored.
**
** Copyright (c) 2004 Ingres Corporation
**
** History:
**      06/12/90 (esd) - Add ps_attr member to posType structure.
**      06/12/90 (esd) - Change 'within' macro to not use spacesize.
**                       This fixes a bug in onPos (see pos.c).
**                       Other pieces of code that invoke the 'within'
**                       macro have been modified where necessary.
**                       I've also modified the VTwithin macro
**                       analogously, even though nobody seems to be
**                       using it.  VTwithin is now identical to
**                       'within'.  (It previously differed only in
**                       dispensing with a superfluous pair of
**                       parentheses).
**	06/10/92 (dkh) - Since 3270 support is no longer needed, use
**			 ps_attr struct member for cross hair/group move
**			 support.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


struct posType
{
	i4		ps_begx, 	/* the beginning and ending x */
			ps_endx,
			ps_begy,	/* same for y */
			ps_endy;
	i4		ps_name;	/* What type of feature */
	i4		ps_mask;        /* what state the position is in */
	i4		*ps_feat;	/* pointer to trim or FIELD structure*/
	struct posType	*ps_column;	/* positions which make up feat */
    	i4		ps_oldx;	/* old x before a move */
    	i4		ps_oldy;	/* old y before a move */
        char            ps_attr;        /* additional attributes for component*/

# define	IS_STRAIGHT_EDGE	1  /* component is a straightedge */
# define	IS_IN_GROUP_MOVE	2  /* component is part of group move */
};

/*
** values for name field
** identifies the type of feature the position is representing
*/

# define	PFIELD		DS_PFIELD
# define	PTRIM		DS_PTRIM
# define	PBLANK		2
# define	PTITLE		3
# define	PDATA		4
# define	PCOLUMN		5
# define	PSPECIAL	DS_SPECIAL	/* is a (TRIM *) */
# define	PTABLE		DS_TABLE	/* is a (FIELD *) */
# define	PBOX		6		/* box/line trim */

/*
** values for mask field
** corresponds to the state of the position.
** When a position gets moved its state is changed
**
** a position can only have one of three states
** either it is NORMAL, it has been moved, or it has had a move undone.
** MOVEDEF.c makes more eloborate use of the mask field.
*/
# define	PNORMAL		0	/* usual state of position */
# define	PMOVE		1	/* position moved to new location */
# define	PUNDO		-1	/* position's adjusted by undo */
# define	PRBFDONE	4	/* was rbf stuff written yet */


typedef struct posType	POS;

# define        within(x, ps)   ((ps)->ps_begx <= (x) && (x) <= (ps)->ps_endx)
# define	vertin(y, ps)	((ps)->ps_begy <= (y) && (y) <= (ps)->ps_endy)

/*
**  New macro to check for overlapping horizontally,  Old version of
**  "within" still needed for finding next and previous features when
**  going between components in the form.  It that case, exact matches
**  must be used.  (dkh)
*/

# define        VTwithin(x, ps) ((ps)->ps_begx <= (x) && (x) <= (ps)->ps_endx)

/*
** a small field is used to pass info between those routines which
** move parts of a field
** it is condensed version of a field
*/
typedef struct smallfield
{
	i4	tx,
		ty,
		tex,
		tey;
	char	*tstr;
	i4	dx,
		dy,
		dex,
		dey;
} SMLFLD;

/*
** the create table structure which is used to help create a table field
*/
typedef struct crtabstr
{
	char	*ct_name;
	char	ct_title;
	FMT	*ct_fmt;
} CRTABLE;
