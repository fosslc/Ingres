/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <me.h>
# include <mm.h>
# include "mmint.h"

/*
 * Name: mmdebug.c - debugging wrappers for MMalloc(), MMfree
 */

typedef struct _MM_DBHDR {
	MM_POOL	*pool;
	i4	size;
	char	*file;
	i4	line;
} MM_DBHDR ;

typedef struct _MM_DBTAIL {
	MM_POOL	*pool;
} MM_DBTAIL ;

PTR
MM_dballoc( MM_POOL	*pool,
	 i4	size,
	 STATUS		*stat,
	 CL_ERR_DESC	*err,
	 char		*file,
	 i4		line )
{
    PTR		obj;
    i4	newsize;
    MM_DBHDR	*hdr;
    MM_DBTAIL	*tail;

    /* Round size - we need alignment for our headers */

    size = ( size + sizeof( MM_DBHDR ) - 1 ) & ~( sizeof( MM_DBHDR ) -1 );

    /* New size is old plus room for our header and tail */

    newsize = sizeof( MM_DBHDR ) + size + sizeof( MM_DBTAIL );

    if( !( obj = MM_alloc( pool, newsize, stat, err ) ) )
	return obj;

    hdr = (MM_DBHDR *)obj;
    hdr->pool = pool;
    hdr->size = size;
    hdr->file = file;
    hdr->line = line;

    obj = (PTR)(hdr + 1);

    tail = (MM_DBTAIL *)((char *)obj + hdr->size );
    tail->pool = pool;

    return obj;
}

STATUS
MM_dbfree( MM_POOL 	*pool,
	PTR 		obj,
	CL_ERR_DESC 	*err )
{
    MM_DBHDR	*hdr;
    MM_DBTAIL	*tail;

    hdr = ((MM_DBHDR *)obj) - 1;

    if( hdr->pool != pool )
	return FAIL;

    tail = (MM_DBTAIL *)((char *)obj + hdr->size );

    if( tail->pool != pool )
	return FAIL;

    return MM_free( pool, (PTR)hdr, err );
}
