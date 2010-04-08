/*
**  Copyright (c) 1986, 2002 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<meprivate.h>
# include	<me.h>


/**
** Name:	mereq.c -	Standard memory allocator entry points.
**
** Description:
**	This file defines:
**
**	MEreqmem	Allocate memory of size <= MAX( u_i4 ).
**	MEreqlng	Allocate memory of size <= MAX( u_i4 ).
**
** History:
**	10-jun-87 (bab)
**		Written as replacements for MEalloc, MEcalloc, MEtalloc
**		and MEtcalloc.
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**      23-may-90 (blaise)
**          Add "include <compat.h>."
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.  Delete forward and external function references;
**              these are now all in either me.h or meprivate.h.
**      08-feb-1993 (pearl)
**          Add #include of me.h.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**	18-oct-1993 (kwatts)
**	    Added a wrapper for IIMEreqmem to restore MEreqmem name for ICL
**	    Smart Disk linkage.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	21-mar-2001 (mcgem01)
**	    Instead of using i4's for pointer sizes, use SIZE_TYPE.
**	22-oct-2002 (somsa01)
**	    Fixed up calling of MEdoAlloc() when passing memory size.
**/



/*{
** Name:	MEreqmem	- Allocate memory of specified size.
**
** Description:
**	Returns a pointer to a block of memory of size bytes.
**	A tag must be specified, although that tag may be 0
**	as a default value.  If the 'clear' parameter is TRUE,
**	then the allocated memory is zeroed.  The fourth parameter
**	may be NULL, in which case no detailed status information
**	is returned.  Returns NULL if an error occurs.
**
** Inputs:
**	tag	id associated with the allocated region of memory.
**	size	Amount of memory to allocate.
**	clear	Specify whether to zero out the allocated memory.
**
** Outputs:
**	status	If all went well, set to OK.
**		Otherwise, set to one of ME_NO_ALLOC,
**					 ME_GONE,
**					 ME_00_PTR,
**					 ME_CORRUPTED,
**					 ME_TOO_BIG.
**		(If status == NULL, then no attempt is made to modify it.)
**
**	Returns:
**		Pointer to the allocated region of memory, or NULL on error.
**
**	Exceptions:
**		<exception codes>
**
** Side Effects:
**	None.
**
** History:
**	10-jun-87 (bab)
**		Written as replacement for MEalloc, MEcalloc, MEtalloc
**		and MEtcalloc.
*/
PTR
MEreqmem(
	u_i2		tag,
	SIZE_TYPE	size,
	bool		clear,
	STATUS		*status)
{
	PTR	block;
	STATUS	istatus;

	/*
	**  When the ME module is re-written, then the contents
	**  of this routine will likewise change.  No further use
	**  of MEdoAlloc should be propagated at that time.  (bab)
	*/
	istatus = MEdoAlloc((i2)tag, (i4)1, size, &block, clear);

	if (status != NULL)
		*status = istatus;

	if (istatus != OK)
		return((PTR)NULL);
	else
		return(block);
}


/*{
** Name:	MEreqlng	- Allocate memory of specified size.
**
** Description:
**	Returns a pointer to a block of memory of size bytes.
**	A tag must be specified, although that tag may be 0
**	as a default value.  If the 'clear' parameter is TRUE,
**	then the allocated memory is zeroed.  The fourth parameter
**	may be NULL, in which case no detailed status information
**	is returned.  Returns NULL if an error occurs.
**	This routine is like MEreqmem, except that the amount of
**	memory allocated may be a larger number of bytes than can
**	fit in a u_i4.
**
** Inputs:
**	tag	id associated with the allocated region of memory.
**	size	Amount of memory to allocate.
**	clear	Specify whether to zero out the allocated memory.
**
** Outputs:
**	status	If all went well, set to OK.
**		Otherwise, set to one of ME_NO_ALLOC,
**					 ME_GONE,
**					 ME_00_PTR,
**					 ME_CORRUPTED,
**					 ME_TOO_BIG.
**		(If status == NULL, then no attempt is made to modify it.)
**
**	Returns:
**		Pointer to the allocated region of memory, or NULL on error.
**
**	Exceptions:
**		<exception codes>
**
** Side Effects:
**	None.
**
** History:
**	10-jun-87 (bab)
**		Written as replacement for MEalloc, MEcalloc, MEtalloc
**		and MEtcalloc.
*/
PTR
MEreqlng(
	u_i2		tag,
	SIZE_TYPE     	size,
	bool		clear,
	STATUS		*status)
{
	PTR	block;
	STATUS	istatus;

	/*
	**  When the ME module is re-written, then the contents
	**  of this routine will likewise change.  No further use
	**  of MEdoAlloc should be propagated at that time.  (bab)
	*/
	istatus = MEdoAlloc((i2)tag, (i4)1, size, &block, clear);

	if (status != NULL)
		*status = istatus;

	if (istatus != OK)
		return((PTR)NULL);
	else
		return(block);
}
/*
**
**	Function:
**		MEreqmem (remains MEreqmem as wrapper to II routine).
**
**	Arguments:
**		char	*source;
**		i4	num_bytes;
**		char	*dest;
**
**	Result:
**		Simply calls IIMEreqmem.
**
**
**	History:
**	18-oct-1993 (kwatts)
**	    Created as wrapper because of name changes.
*/
#ifdef MEreqmem
#undef MEreqmem
PTR
MEreqmem(
	u_i2		tag,
	SIZE_TYPE	size,
	bool		clear,
	STATUS		*status)
{
    return(IIMEreqmem(tag, size, clear, status));
}
#endif
