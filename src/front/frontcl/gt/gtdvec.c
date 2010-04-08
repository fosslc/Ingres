/*
**    Copyright (c) 2004 Ingres Corporation
*/

/*# includes */
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<dvect.h>
# include	"ergt.h"

/**
** Name:    gtdvec	data vector routines.
**
** Description:
**	Routines to build and destroy named vectors of data in memory.
**	Calling routines access information about these vectors, and
**	array pointers to the data space through the GR_DVEC structure.
**	These routines are simply space allocators - they do not fill
**	in data.  This file defines:
**
**	GTdvcreate	create a data vector.
**	GTdvdestroy	destroy a data vector.
**	GTdvrestart	destroy all data vectors.
**	GTdvscan	scan list of currently available vectors.
**	GTdvfetch	fetch a specific vector.
**
** History:
**	20-may-88 (bruceb)
**		Changed FEtalloc calls to FEreqmem calls.
**	08-jun-88 (bruceb)
**		Sigh!  Do the above properly.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

static GR_DVEC *Vlist = NULL;	/* list of current data vectors */

/*{
** Name:	GTdvcreate	- create a named data vector
**
** Description:
**	allocates space for storing a named vector of data.
**
** Inputs:
**	name	- name for new vector
**	type	- type of data contained in new vector
**	len	- number of elements in new vector
**
** Outputs:
**	Returns:
**		Pointer to newly created vector.
**	Exceptions:
**		Syserr if memory can't be allocated.
**
** Side Effects:
**
** History:
**	8/85 (rlm)	- written
*/

GR_DVEC *
GTdvcreate (name,type,len)
char *name;
i4  type;
i4  len;
{
	i2 tag;
	GR_DVEC *ptr;
	i4 erarg;

	tag = FEgettag();

	if ((ptr = (GR_DVEC *)FEreqmem((u_i4)tag, (u_i4)sizeof(GR_DVEC),
		(bool)FALSE, (STATUS *)NULL)) == NULL)
	{
		GTsyserr (S_GT000D_DV_alloc,0);
	}

	ptr->tag = tag;
	ptr->len = len;
	ptr->link = Vlist;
	Vlist = ptr;
	ptr->type = type;
	if ((ptr->name = FEtsalloc(tag,name)) == NULL)
		GTsyserr (S_GT000D_DV_alloc,0);

	switch (type & DVEC_MASK)
	{
	case DVEC_I:
		if ((ptr->ar.idat = (i4 *)FEreqmem((u_i4)tag,
			(u_i4)(len * sizeof(i4)), (bool)FALSE,
			(STATUS *)NULL)) == NULL)
		{
			GTsyserr (S_GT000D_DV_alloc, 0);
		}
		break;
	case DVEC_F:
		if ((ptr->ar.fdat = (float *)FEreqmem((u_i4)tag,
			(u_i4)(len * sizeof(float)), (bool)FALSE,
			(STATUS *)NULL)) == NULL)
		{
			GTsyserr (S_GT000D_DV_alloc, 0);
		}
		break;
	case DVEC_S:
		if ((ptr->ar.sdat.val = (i4 *)FEreqmem((u_i4)tag,
			(u_i4)(len * sizeof(i4)), (bool)FALSE,
			(STATUS *)NULL)) == NULL)
		{
			GTsyserr (S_GT000D_DV_alloc, 0);
		}
		if ((ptr->ar.sdat.str = (char **)FEreqmem((u_i4)tag,
			(u_i4)(len * sizeof(char *)), (bool)FALSE,
			(STATUS *)NULL)) == NULL)
		{
			GTsyserr (S_GT000D_DV_alloc, 0);
		}
		break;
	default:
		erarg = type;
		GTsyserr (S_GT000E_DV_Bad_Type, 1, (PTR) &erarg);
		break;
	}

	return (ptr);
}

/*{
** Name:	GTdvdestroy - destroy an existing vector
**
** Description:
**	allocates space for storing a named vector of data.
**
**	NOTE - GTdsdestroy is called to remove any Cchart datasets
**	we may have lying around which involve this vector.
**
** Inputs:
**	name	- name of vector
**
** History:
**	8/85 (rlm)	- written
*/

void
GTdvdestroy (name)
char *name;
{
	GR_DVEC *ptr,*optr;

	/* find it, and keep track of predecessor for relinking */
	optr = NULL;
	for (ptr = Vlist; ptr != NULL &&
				STcompare(name,ptr->name) != 0; ptr = ptr->link)
		optr = ptr;

	/* call GTdsdestroy, unlink from list and free tag*/
	if (ptr != NULL)
	{
		GTdsdestroy (name);
		if (optr == NULL)
			Vlist = ptr->link;
		else
			optr->link = ptr->link;
		FEfree (ptr->tag);
	}
}

/*{
** Name:	GTdvrestart	- delete all data vectors
**
** Description:
**	destroys all data vectors
**
**	NOTE - GTdsdestroy is called to remove any Cchart datasets
**	we may have lying around which involve each vector.
**
** Inputs:
**
** History:
**	8/85 (rlm)	- written
*/

void
GTdvrestart ()
{
	GR_DVEC *ptr, *optr;

	for (ptr = Vlist; ptr != NULL; ptr = optr)
	{
		optr = ptr->link;
		GTdsdestroy (ptr->name);
		FEfree (ptr->tag);
	}

	Vlist = NULL;
}

/*{
** Name:	GTdvscan	- scan list of available vectors
**
** Description:
**	scan list of avalable vectors, returnin "first", "next" or NULL
**
**	eg:
**		for (ptr = GTdvscan(NULL); ptr != NULL; ptr = GTdvscan(ptr))
**
** Inputs:
**	ptr	- current list element, NULL to obtain head of list.
**
** Outputs:
**	Returns:
**		Pointer to newly created vector.
**	Exceptions:
**		Syserr if memory can't be allocated.
**
** Side Effects:
**
** History:
**	8/85 (rlm)	- written
*/

GR_DVEC *
GTdvscan (ptr)
GR_DVEC *ptr;
{
	if (ptr == NULL)
		return (Vlist);
	return (ptr->link);
}

/*{
** Name:	GTdvfetch	- fetch a named data vector
**
** Description:
**	fetches pointer to a specified vector.
**
** Inputs:
**	name	- name for vector
**
** Outputs:
**	Returns:
**		Pointer to vector, NULL for failure (doesn't exist)
**
** History:
**	8/85 (rlm)	- written
*/

GR_DVEC *
GTdvfetch (name)
char *name;
{
	GR_DVEC *ptr;

	for (ptr = Vlist; ptr != NULL && STcompare(name,ptr->name) != 0;
								ptr = ptr->link)
		;

	return (ptr);
}
