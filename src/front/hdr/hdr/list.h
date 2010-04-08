/*
**	Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)list.h	30.4	1/24/85"; */

/*
** list.h
** contains declaraions for list used throughout editor
*/

# include	<fedscnst.h>

struct listType
{
	struct listType	*lt_next;
	i4	lt_utag;
	union
	{
		char		*lt_vfeat;
		struct posType	*lt_vpos;
		struct trmstr	*lt_vtrim;
		struct fldstr	*lt_vfield;
		struct nodeType	*lt_vnode;
	} lt_var;
};

# define	lt_pos		lt_var.lt_vpos
# define	lt_feat		lt_var.lt_vfeat
# define	lt_trim		lt_var.lt_vtrim
# define	lt_field	lt_var.lt_vfield
# define	lt_node		lt_var.lt_vnode
# define	LPOS	DS_PPOS
# define	LFEAT	DS_STRING
# define	LTRIM	DS_PTRIM
# define	LFIELD	DS_PFIELD
# define	LNODE	DS_PVFNODE

typedef struct listType	LIST;

# define		LNIL	((LIST *) 0)
