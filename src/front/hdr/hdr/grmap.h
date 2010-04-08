/*
**	Copyright (c) 2004 Ingres Corporation
*/
/**
** Name:	grmap.h -	Graphics System Data Map Module Definitions.
**
** Description:
**	Defines the types and constants needed to support data mapping
**	for graphs supported by the Graphics System.
**
** History:	
 * Revision 4003.1  86/05/19  09:56:25  root
 * 'new' vigraph files.
 * 
 * Revision 4002.2  86/05/08  11:55:05  rpm
 * vms/unix integration.
 * 
**		Revision 4002.1  86/04/15  21:45:16  root
**		4.0/02 certified codes
** 
**		Revision 40.101  86/04/08  13:52:22  robin
**		Change 'spread' from a bool to be a flag in 'flags', and added a 'nosort'
**		flag.
**		
**		Revision 40.100  86/02/27  08:28:55  bobm
**		BETA CHECKPOINT
**		
**		Revision 40.0  86/02/03  14:52:07  wong
**		Initial revision.  (Separated from "gropt.h".)
**		
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*}
** Name:	GR_SORT -	Graphics System Data Map Sort Entry.
**
** Description:
**	Describes a sort specification for the Graphics System Data Map.
**
** History:
**	12/85 (rlm) -- Defined.
*/
typedef struct
{
	char	*s_colname;	/* column name */
	char	s_dir;		/* 'a' or 'd' */
# define	S_ASCEND '+'
# define	S_DESCEND '-'
} GR_SORT;

# define	MAX_SORT	10	/* max no. columns allowed to sort */
# define	MAXSORTSTRING	120	/* max size of sort string buffer */

/*}
** Name:	GR_MAP -	Graphics System Data Map Structure.
**
** Description:
**	Defines the data map for a graph.
**
** History:
**	12/85 (rlm) -- Defined.
*/
typedef struct
{
	i4	id;			/* object (database) id */
	char	*gr_view;		/* view name */
	char	*gr_name;		/* graph name */
	char	*gr_type;		/* graph type */
	char	*gr_desc;		/* graph description */
	char	*indep;			/* independent axis column */
	char	*dep;			/* dependent axis column */
	char	*series;		/* series column */
	char	*label;			/* data label column */
	u_i4	flags;			/* flags for sorting, spreadsheet */
	i4	ntosort;		/* number of columns to sort on */
	GR_SORT	sortcols[MAX_SORT];	/* columns to sort on, in order */
} GR_MAP;

/*
**  Defines for the flags item in the GR_MAP structure 
*/

# define	MAP_SPREAD	0x001	/* spread sheet data model */
# define	MAP_NOSORT	0x002	/* data should not be sorted */

