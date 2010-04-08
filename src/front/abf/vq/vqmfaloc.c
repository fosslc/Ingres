
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<ft.h>
# include	<fmt.h>
# include	<frame.h>
# include	<ooclass.h>
# include	<er.h>
# include	<abclass.h>
# include	<me.h>
# include	<ex.h>
# include	<st.h>

# include	<metafrm.h>

/**
** Name:	vqmfaloc -	allocate a metaframe components
**
** Description:
**	Allocate the various components of a metaframe structure.

** This file defines:
**
**	IIVQamfAllocMF		allocate a metaframe 
**	IIVQavqAllocVQ 		allocate metaframe visual query components
**	IIVQ_MAlloc 		allocate memory on the appl tab 
**	IIVQ_VAlloc 		allocate memory on the vqtag
**	IIVQfvtFreeVqTag 	free the objects allocated on the vqtag
**
** History:
**	03/22/89 (tom) - written
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */

/* GLOBALDEF's */

/* extern's */
FUNC_EXTERN PTR IIVQ_MAlloc();
FUNC_EXTERN PTR IIVQ_VAlloc();

GLOBALREF APPL *IIVQappl;

/* static's */
static PTR _alloc_mem();

static TAGID vqtag;




#if 0	/* !! no need for this function right now */

static TAGID mftag;




/*{
** Name:	IIVQam_AllocMF	- allocate a metaframe struct
**
** Description:
**	Allocate memory for a METAFRAME structure and it's associated
**	user frame, and the array to contain menu item struct pointers.
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**		METAFRAME *	- ptr to the metaframe
**		
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	03/22/89 (tom) - written
*/
METAFRAME *
IIVQamfAllocMF()
{
	register METAFRAME *mf;
	register USER_FRAME *uf;
	register PTR p;

	if (mftag == NULL)
		mftag = FEgettag();

	uf = (USER_FRAME*)IIVQ_MAlloc(sizeof(USER_FRAME), (char*)NULL);

	mf = (METAFRAME*)IIVQ_MAlloc(sizeof(METAFRAME), (char*)NULL);

	if (uf == (USER_FRAME*)NULL || mf == (METAFRAME*)NULL)
		return (METAFRAME*)(NULL);

	p = IIVQ_MAlloc(sizeof(MFMENU*) * MF_MAXMENUS, (char*)NULL);
	if (p == NULL)
		return (METAFRAME*)(NULL);
	mf->menus = (MFMENU**)p;

	uf->mf = mf;
	mf->apobj = (OO_OBJECT*)uf;

	return (mf);
}



/*{
** Name:	IIVQavqAllocVQ 	- allocate metaframe vq  
**
** Description:
**	Allocate memory for METAFRAME components associated with
**	the visual query.
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**		bool	- TRUE = success, else FALSE	
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	03/22/89 (tom) - written
*/
bool
IIVQavqAllocVQ(mf)
register METAFRAME *mf;
{
	register TAGID	tag;
	register PTR p;

	/* allocate the pointer arrays */

	p = IIVQ_VAlloc(sizeof(MFTAB*) * MF_MAXTABS, (char*)NULL);
	if (p == NULL)
		return (FALSE);
	mf->tabs = (MFTAB**)p;

	p = IIVQ_VAlloc(sizeof(MFJOIN*) * MF_MAXJOINS, (char*)NULL);
	if (p == NULL)
		return (FALSE);
	mf->joins = (MFJOIN**)p;

	p = IIVQ_VAlloc(sizeof(MFESC*) * MF_MAXESCS, (char*)NULL);
	if (p == NULL)
		return (FALSE);
	mf->escs = (MFESC**)p;

	p = IIVQ_VAlloc(sizeof(MFVAR*) * MF_MAXVARS, (char*)NULL);
	if (p == NULL)
		return (FALSE);
	mf->vars = (MFVAR**)p;

	return (TRUE);
}




#endif






/*{
** Name:	IIVQ_MAlloc 	- allocate some memory on appl tag 
**
** Description:
**	This function will allocate memory on the application's tag.
**	This memory will only be freed when the user exits the
**	edit of the current application.
**
** Inputs:
**	i4	size;	- size of memory to allocate
**	char	*str;	- ptr to string
**
** Outputs:
**	Returns:
**		PTR	- pointer to the memory
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	03/22/89 (tom) - written
*/
PTR
IIVQ_MAlloc(size, str)
register i4	size;
register char	*str;
{

	return  (_alloc_mem(IIVQappl->data.tag, size, str));
}
/*{
** Name:	IIVQ_VAlloc 	- allocate some memory on vq tag 
**
** Description:
**	Allocate some memory on vqtag. if string is given
**	then allocate and copy the string.. else just allocate
**	and return the given size.
**
** Inputs:
**	i4	size;	- size of memory to allocate
**	char	*str;	- ptr to string
**
** Outputs:
**	Returns:
**		PTR	- pointer to the memory
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	03/22/89 (tom) - written
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/
PTR
IIVQ_VAlloc(size, str)
register i4	size;
register char	*str;
{

	if (vqtag == 0)
		vqtag = FEgettag();

	return  (_alloc_mem(vqtag, size, str));
}

/*{
** Name:	_alloc_mem 	- allocate some memory 
**
** Description:
**	Allocate some memory.
**
** Inputs:
**	TAGID	tag;	- tag to allocate memory on
**	i4	size;	- size of memory to allocate if there isn't a string
**	char	*ptr;	- pointer to string to allocate
**
** Outputs:
**	Returns:
**		PTR	- pointer to the memory
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	03/22/89 (tom) - written
*/
static PTR
_alloc_mem(tag, size, str)
TAGID	tag;
i4	size;
register char *str;
{

	register char *p;

	if (str != (char*)NULL)
		size = STlength(str) + 1;

	if ( (p =  FEreqmem((u_i4)tag, (u_i4)size, TRUE, (STATUS*) NULL))
			== NULL)
	{
		EXsignal(EXFEMEM, 0);
	}

	if (str != (char*)NULL)
		STcopy(str, (char*)p);

	return (p);
}

/*{
** Name:	IIVQfvtFreeVqTag 	- free the visual query component tag
**
** Description:
**	Allocate memory for METAFRAME components associated with
**	the frame flow diagram (menu pointer array).
**	This is allocated on the mftag.
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**		bool	- TRUE = success, else FALSE	
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	03/22/89 (tom) - written
*/
VOID
IIVQfvtFreeVqTag()
{
	if (vqtag)
		FEfree(vqtag);
}
