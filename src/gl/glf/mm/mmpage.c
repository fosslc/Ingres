/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<mm.h>
# include	<me.h>
# include	"mmint.h"

/*
**
** History:
**
**	22-apr-1996 (lawst01)
**	   Windows 3.1 port changes - commented out printf debug? statement
**
** Name: mmstpage.c - allocator alloc/free/rm for the page allocator
**
** Description:
**
** 	Implements MM_PAGE_STRATEGY:
**
**	MM_pag_alloc rounds requests up to a page size and requests the
**	proper number of pages from MEget_pages.  It never accesses its
**	parent_tag.
**
**	For each allocated set of pages, MM_pag_alloc fills a slot in a
**	MM_INDEX_PAGE with an MM_PAGE descriptor.  MM_INDEX_PAGEs are
**	each a single page and chained onto the pool's alloc_list.  The
**	pages listed on the MM_INDEX_PAGEs plus the MM_INDEX_PAGEs
**	themselves comprise the blocks requested of the parent
**	(MEget_pages).
**
**	When MM_pag_free frees an allocated set of pages, it scans the
**	MM_INDEX_PAGEs for the right MM_PAGE descriptor.  The pages are
**	freed with MEfree_pages, and then the MM_PAGE for the last set
**	of pages allocated is moved into the newly emptied slot.  If
**	this moving empties the last MM_INDEX_PAGE, it too is freed.
**
** History:
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores.
**	07-oct-2004 (thaju02)
**	    Change pages to SIZE_TYPE.
*/

typedef struct _MM_PAGE MM_PAGE;
typedef struct _MM_INDEX_PAGE MM_INDEX_PAGE;

/*
 * Name: MM_PAGE - description of an allocated set of pages
 */

struct _MM_PAGE {
	PTR		ptr;
	i4		size;
} ;

/*
** Name: MM_INDEX_PAGE - keeps track of allocated sets of pages
*/

struct _MM_INDEX_PAGE {
	MM_BLOCK	hdr;
	i4		slots_max;
	i4		slots_used;
	MM_PAGE		pages[1];	/* acutally [slots_max] */
} ;

STATUS
MM_pag_mk(
	MM_POOL		*pool,
	CL_ERR_DESC 	*err )
{
	pool->service_size = ME_MPAGESIZE;
	pool->service_cost = 0;
	return OK;
}

PTR
MM_pag_alloc(
	MM_POOL		*pool,
	i4		size,
	STATUS 		*stat,
	CL_ERR_DESC 	*err )
{
    PTR			p;
    SIZE_TYPE		pages;
    MM_PAGE		*page;
    MM_INDEX_PAGE	*ixpage = (MM_INDEX_PAGE *)pool->alloc_list;

    /*
    printf("page %d bytes\n", size );
    */

    /* Ensure room on a MM_INDEX_PAGE to note this allocation */

    if( !ixpage || ixpage->slots_used == ixpage->slots_max )
    {
	/* Need another MM_INDEX_PAGE */

	pages = 1;

# define USEME
# ifdef USEME
	*stat = MEget_pages( 0, pages, (char *)NULL, &p, &pages, err ) ;
	if( *stat )
           {
	    return 0;
           }
# else
	if( !( p = malloc( pages * ME_MPAGESIZE ) ) )
           {
	    return 0;
           }
# endif

	/* Chain onto front of alloc_list */

	ixpage = (MM_INDEX_PAGE *)p;
	ixpage->slots_used = 0;
	ixpage->slots_max = ( ME_MPAGESIZE - sizeof( MM_INDEX_PAGE ) ) 
			  / sizeof( MM_PAGE ) + 1;
	ixpage->hdr.size = ME_MPAGESIZE;
	ixpage->hdr.next = pool->alloc_list;
	pool->alloc_list = &ixpage->hdr;
    }

    /* Now allocate requested pages */

    pages = ( size + ME_MPAGESIZE - 1 ) / ME_MPAGESIZE;

# ifdef USEME
    *stat = MEget_pages( 0, pages, (char *)NULL, &p, &pages, err );
    if( *stat )
       {
	return 0;
       }
# else
    if( !( p = malloc( pages * ME_MPAGESIZE ) ) )
       {
	return 0;
       }
# endif

    /* Note allocated pages */

    page = &ixpage->pages[ ixpage->slots_used++ ];
    page->size = pages;
    page->ptr = p;

    return p;
}

STATUS
MM_pag_free( 
	MM_POOL		*pool,
	PTR 		obj,
	CL_ERR_DESC 	*err )
{
    MM_INDEX_PAGE	*ixpage = (MM_INDEX_PAGE *)pool->alloc_list;
    MM_PAGE		*page;
    STATUS		status;
    i4			i;

    /* Laboriously find allocated page */

    for( ; ixpage; ixpage = (MM_INDEX_PAGE *)ixpage->hdr.next )
	for( page = ixpage->pages, i = ixpage->slots_max; i; i--, page++ )
	    if( page->ptr == obj )
    {
	/* Free the thing */

# ifdef USEME
	if( status = MEfree_pages( page->ptr, page->size, err ) )
           { 
	    return status;
           }
# else
	free( page->ptr );
# endif

	/* Move the last entry on the head ixpage to the emptied spot */

    	ixpage = (MM_INDEX_PAGE *)pool->alloc_list;
	*page = ixpage->pages[ --ixpage->slots_used ];

	/* If ixpage empty, free the ixpage */

	if( !ixpage->slots_used )
	{
	    pool->alloc_list = ixpage->hdr.next;

# ifdef USEME
	    if( status = MEfree_pages( (PTR)ixpage, ixpage->hdr.size, err ) )
               {
		return status;
               }
# else
	    free( ixpage );
# endif
	}

	return OK;
    }

    /* Freeing object not allocated (by us) */

    return FAIL; 
}

STATUS
MM_pag_rm( 
	MM_POOL		*pool,
	CL_ERR_DESC 	*err )
{
    MM_INDEX_PAGE	*ixpage = (MM_INDEX_PAGE *)pool->alloc_list;
    MM_INDEX_PAGE	*next_ixpage;
    MM_PAGE		*page;
    STATUS		status;
    i4			i;

    /* Free all pages requested of MEget_pages */

    while( ixpage )
    {
	/* Get next in list so we can free this page */

	next_ixpage = (MM_INDEX_PAGE *)ixpage->hdr.next;

	/* Free pages listed on this page */

	for( page = ixpage->pages, i = ixpage->slots_max; i; i--, page++ )
        {
# ifdef USEME
	    status = MEfree_pages( page->ptr, page->size, err );
	    if( status )
               {
		return status;
               } 
# else
	    free( page->ptr );
# endif
        }

	/* free the ixpage */

	if( status = MEfree_pages( (PTR)ixpage, ixpage->hdr.size, err ) )
           {
	    return status;
           }

	/* Onto next page */

	ixpage = next_ixpage;
    }

    return OK; 
}

MM_POOL
MM_pool_page[] =
    {
	0, 0, 0, ME_MPAGESIZE, 0, 
	ME_MPAGESIZE, 0, 3,
	0, MM_pag_alloc, MM_pag_free, MM_pag_rm,
	0, 0, 0, 0
    } ;
