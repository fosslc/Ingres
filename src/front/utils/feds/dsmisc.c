/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>
# include	<ds.h>
# include	<ex.h>
# include	<lo.h>
# include	<si.h>
# include	"dsmapout.h"

/**
** Name:	dsmisc.c -- contains miscellaneous routines for the shared DS.
**
** History:
**	85/02/13  09:38:48  ron
**	Initial version of new DS routines from the CFE project.
**	17-aug-91 (leighb) DeskTop Porting Change:
**		func decl of MEptradd only if MEptradd is not a macro.
**	26-may-92 (leighb) DeskTop Porting Change:
**		changed MSDOS ifdefs to PMFE ifdefs to avoid problems on OS/2.
**	02-dec-93 (smc)
**		Bug #58882
**		Removed non-portable truncating cast.
**	15-jan-1996 (toumi01; from 1.1 axp_osf port)
**		(schte01) added 640502 change 06-apr-93 (kchin)
**		Added a couple of changes to fix formindex problem due to
**		pointer being truncated when porting to 64-bit platform,
**		axp_osf.
**	28-feb-2000 (somsa01)
**		LOsize() can now handle >2GB sizes. However, until we
**		actually have clients which will create these front-end
**		files >2GB, we will force the output of LOsize() here to
**		an i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	31-jul-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
*/

GLOBALREF DSHASH	**IIFShashtab;
GLOBALREF DSTEMPLATE	**IIFStemptab;	/* template table */
GLOBALREF i4		IIFStabsize;	/* size of template table */
GLOBALREF STATUS	IIFSretstat;

PTR IIFSfindoff(PTR s);

#ifdef PMFE
#ifndef MEptradd
char 	*MEptradd();
#endif
#endif

# define	NODEBLOCK	20

static DSHASH	*Ndblock = NULL;
static i4	Ndavail = 0;

VOID
IIFSndinit()
{
	Ndavail = 0;
}

PTR
IIFSfindoff ( s )
PTR	s;
{
	register DSHASH	*last, *cur;
	register DSHASH	*node;
	i4		index;
	STATUS  	stat_ret;

#ifdef PMFE
	/* MEptradd normalizes the address */
	s = (PTR) MEptradd((char *)s, (i4) 0);
#endif

	index = hash(s);

	if (s != NULL)
	{
		for (last = NULL, cur = *(IIFShashtab + index); cur != NULL;
			last = cur, cur = cur->next)
		{
			if (s == (PTR)cur->addr)
			{
				if (cur->offset == 0)
				{ /* cycling references occurrd */
					EXsignal(DSCIRCL, 0, 0);
				}
				else
				{
					return cur->offset;
				}
			}
			if (cur->addr > s)
				break;
		}
		if (Ndavail <= 0)
		{
			Ndblock = (DSHASH *) MEreqmem(DSMEMTAG,
				NODEBLOCK*(sizeof(DSHASH)), 0, &stat_ret
			);
			if ( Ndblock == NULL )
				return 0;
			Ndavail = NODEBLOCK;
		}
		node = Ndblock;
		++Ndblock;
		--Ndavail;
		node->addr = s;
		node->next = NULL;
		node->offset = 0;
		if ((last == NULL) &&  (cur == NULL))
		{
			/* insert in the front of the linked list */
			*(IIFShashtab + index) = node;
		}
		else if (cur == NULL)
		{
			/* insert in the end of the linked list */
			last->next = node;
		}
		else
		{
			/* insert in the middle of the linked list */
			node->next = cur;
			if (last == NULL)
				*(IIFShashtab + index) = node;
			else
				last->next = node;
		}
	}
	return 0;
}

VOID
IIFSinsert(s, offset)
PTR	s;
PTR	offset;
{
	DSHASH	*node;
	i4	index;

#ifdef PMFE
	/* MEptradd normalizes the address */
	s = (PTR) MEptradd((char *)s, (i4)0);
#endif

	index = hash(s);

	for (node = *(IIFShashtab + index); node != NULL ; node = node->next)
	{
		if (s == (PTR)node->addr)
		{
			node->offset = offset;
			return;
		}
	}
	EXsignal(INSTERR, 0, 0);
}


DSTEMPLATE	*
IIFStplget(ds_id)
i4	ds_id;
{
	i4		size = IIFStabsize;
	DSTEMPLATE	*template;

	while (--size >= 0)
	{
		template = IIFStemptab[size];
		if (template->ds_id == ds_id)
			return(template);
	}
	EXsignal(IDNTFND, 0, 0);	/* template not found */
}

STATUS
IIFSexhandl(arg)
EX_ARGS	*arg;
{
	/*
	** We just longjmp out.
	*/
	IIFSretstat = FAIL;
	switch(arg->exarg_num)
	{
	  case OPENERR:
		SIfprintf(stderr, "shared data region open error\n");
		break;

	  case DSCIRCL:
		SIfprintf(stderr, "cycling reference data structure\n");
		break;

	  case IDNTFND:
		SIfprintf(stderr, "data structure id not found\n");
		break;

	  case INSTERR:
		SIfprintf(stderr, "insert offset err\n");
		break;

	  case MXSZERR:
		SIfprintf(stderr, "maximum size exceeded (2GB)\n");
		break;

# ifdef PCINGRES
	  case ME_OUTOFMEM:
		IIFSretstat = ME_GONE;
		break;
# endif /* PCINGRES */
	}
	return (EXDECLARE);
}

/* get the size of the written data structure file */
i4
IIFSfsize(locp)
LOCATION	*locp;
{
	OFFSET_TYPE	loc_size;

	LOsize(locp, &loc_size);
	if (loc_size <= MAXI4)
	    return((i4)loc_size);

	EXsignal(MXSZERR, 0, 0);
}
