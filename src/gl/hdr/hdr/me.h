/*
** Copyright (c) 2004 Ingres Corporation
**
*/
# ifndef ME_HDR_INCLUDED
# define ME_HDR_INCLUDED

#include    <mecl.h>

/**CL_SPEC
** Name:	ME.h	- Define ME function externs
**
** Specification:
**
** Description:
**	Contains ME function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	26-jul-93 (tyler)
**	    Added prototypes for MEgettag() and MEfreetag().
**	8/93 (Mike S) Remove dofree from MEfreetag.
**	24-Aug-1993 (fredv)
**		Added #ifdef CL_PROTOTYPE to MEfreetag(); otherwise,
**		metag.c won't compile..
**	18-oct-1993 (kwatts)
**	    Added prototypes for non-II versions of MEcopy and MEreqmem since
**	    these are referenced by the ICL Smart Disk module.
**	03-jun-1996 (canor01)
**	    Added prototypes for thread-local storage functions.
**	30-sep-1996 (canor01)
**	    Only use MEtls prototypes if they are not implemented as
**	    macros.
**	22-nov-1996 (canor01)
**	    Remove prototypes for CL-specific thread-local storage functions.
**	    They will be renamed from "MEtls..." to "ME_tls..." to avoid
**	    conflict with similar functions in GL.  They will be prototyped
**	    in meprivate.h.
**	 2-Dec-96 (gordy)
**	    Implemented GL version of thread local storage.
**      01-Oct-97 (fanra01)
**          Add MEcopylng function prototype.
**	19-jan-1998 (canor01)
**	    Remove CL_DONT_SHIP_PROTOS.
**	03-feb-1998 (canor01)
**	    Add MEfilllng function prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	17-oct-2000 (somsa01)
**	    Added prototype for MEmove().
**	09-feb-2001 (somsa01)
**	    Changed type of size to be SIZE_TYPE for MEreqlng().
**	23-mar-2001 (mcgem01)
**	    IIMEreqmem is required for backward compatibility.
**	02-apr-2001 (mcgem01)
**	    Add back missing define for IIMEmove and prototype for
**	    MEreqlng as both are required for backward compatability.
**	07-oct-2004 (thaju02)
**	    Change pages, allocated_pages, num_of_pages to SIZE_TYPE.
**	11-May-2009 (kschendel) b122041
**	    Change MEfree pointer arg to void * to stop countless cc warnings.
**	    Other ME functions should get the same treatment at some point.
**	14-May-2009 (kschendel) b122041
**	    Change MEtls-destroy to match the above.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill CL_PROTOTYPED (always on now).
**/

/*
** Thread-local-storage object key.
*/

typedef	PTR	METLS_KEY;


/*
** Global Functions.
*/

#define MEadvise IIMEadvise
FUNC_EXTERN STATUS MEadvise(
	i4 mode
);

FUNC_EXTERN STATUS MEfree(
	void	*block
);

#define MEfree_pages IIMEfree_pages
FUNC_EXTERN STATUS MEfree_pages(
	PTR		address,
	SIZE_TYPE	num_of_pages,
	CL_SYS_ERR	*err_code
);

#define MEget_pages IIMEget_pages
FUNC_EXTERN STATUS MEget_pages(
	i4		flag,
	SIZE_TYPE	pages,
	char		*key,
	PTR		*memory,
	SIZE_TYPE	*allocated_pages,
	CL_SYS_ERR	*err_code
);

#define MEmove IIMEmove
FUNC_EXTERN VOID MEmove(
	u_i2	slen,
	PTR	source,
	char	pad_char,
	u_i2	dlen,
	PTR	dest
);

#define MEprot_pages IIMEprot_pages
FUNC_EXTERN STATUS MEprot_pages(
	PTR		addr,
	SIZE_TYPE	pages,
	i4		protection
);

#define MEreqlng IIMEreqlng
FUNC_EXTERN PTR MEreqlng(
        u_i2            tag,
        SIZE_TYPE	size,
        bool            clear,
        STATUS          *status
);

#define MEreqmem IIMEreqmem
FUNC_EXTERN PTR MEreqmem(
	u_i2		tag,
	SIZE_TYPE	size,
	bool		clear,
	STATUS		*status
);

#define MEshow_pages IIMEshow_pages
FUNC_EXTERN STATUS MEshow_pages(
	STATUS		(*func)(),
	PTR		*arg_list,
	CL_SYS_ERR	*err_code
);

#define MEsize IIMEsize
FUNC_EXTERN STATUS MEsize(
	PTR		block,
	SIZE_TYPE	*size
);

#define MEsmdestroy IIMEsmdestroy
FUNC_EXTERN STATUS MEsmdestroy(
	char		*key,
	CL_SYS_ERR	*err_code
);

#define MEtfree IIMEtfree
FUNC_EXTERN STATUS MEtfree(
	u_i2	tag
);

#define MEgettag IIMEgettag
FUNC_EXTERN u_i2 MEgettag(
	VOID	
);

#define MEfreetag IIMEfreetag
FUNC_EXTERN STATUS MEfreetag(
	u_i2	tag
);

#define MEtls_create IIMEtls_create
FUNC_EXTERN STATUS
MEtls_create( METLS_KEY *tls_key );

#define MEtls_set IIMEtls_set
FUNC_EXTERN STATUS
MEtls_set( METLS_KEY *tls_key, PTR tls_value );

#define MEtls_get IIMEtls_get
FUNC_EXTERN STATUS
MEtls_get( METLS_KEY *tls_key, PTR *tls_value );

#define MEtls_destroy IIMEtls_destroy
FUNC_EXTERN STATUS
MEtls_destroy( METLS_KEY *tls_key, STATUS (*destructor)( void * ) );

#ifdef xDEBUG
VOID	MEshow(void);
#endif
#ifdef xPURIFY
VOID	IIME_purify_tag_exit(void);
#endif

# endif /* ME_HDR_INCLUDED */
