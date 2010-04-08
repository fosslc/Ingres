# include	<compat.h>
# include	<gl.h>
# include	<si.h>
# include	<ds.h>
# include	<me.h>
# include	<ex.h>
# include	<lo.h>
# include	"dsmapout.h"

GLOBALDEF	DSHASH		**DShashtab;
GLOBALDEF	DSTEMPLATE	**DStemptab;	/* template table */
GLOBALDEF	i2	TAB_SZ;			/* size of template table */
/*
**  History:
**      11-aug-93 (ed)
**          added missing includes
**	26-oct-01 (kinte01)
**	    clean up compiler warnings
**
*/
findoffset(s)
i4	s;
{
	DSHASH	*last, *cur;
	DSHASH	*node;
	i4	index = hash(s);

	if (s != NULL)
	{
 	for(last = NULL, cur = *(DShashtab + index); cur != NULL; last = cur, cur = cur->next)
	{
		if (s == cur->addr)
			if (cur->offset == 0)	/* cycling references occurrd */
				EXsignal(DSCIRCL, 0, 0); 
			else
				return(cur->offset);
		if (cur->addr > s)
		break;
	}
	node = (DSHASH *) MEreqmem(0, sizeof(DSHASH), FALSE, NULL);
	node->addr = s;
	node->next = NULL;
	node->offset = 0; 
	if ((last == NULL) &&  (cur == NULL))
	/* insert in the front of the linked list */
			*(DShashtab + index) = node;
	else if (cur == NULL)
	/* insert in the end of the linked list */
	last->next = node;
	else {
	/* insert in the middle of the linked list */
			node->next = cur;
			if (last == NULL)
				*(DShashtab + index ) = node;
			else
				last->next = node;
	      }
	}
	return(NULL);
}
	
insert_offset(s, offset)
i4	s;
i4	offset;
{
	DSHASH	*node;
	for (node = *(DShashtab + hash(s)); node != NULL ; node = node->next)
		if (s == node->addr)
		{
			node->offset = offset;
			return;
		}
	EXsignal(INSTERR, 0, 0);
}


DSTEMPLATE	*gettemp(ds_id)
i4	ds_id;
{
	i2	size = TAB_SZ;
	DSTEMPLATE	*template;

	while (--size >= 0)
	{
		template = DStemptab[size];
		if (template->ds_id == ds_id)
		return(template);
	}
	EXsignal(IDNTFND, 0, 0);	/* template not found */
}

dshandl(arg)
EX_ARGS	*arg;
{
	/*
	** We just longjmp out.
	*/
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
		SIfprintf(stderr, "insert_offset err\n");
		break;
	}
	return (EX_DECLARE);
}

/* get the size of the written data structure file */
DSfsize(locp)
LOCATION	*locp;
{
	OFFSET_TYPE	loc_size;

	LOsize(locp, &loc_size);
	return(loc_size);
}

