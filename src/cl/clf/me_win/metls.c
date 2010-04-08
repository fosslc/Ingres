/*
** Copyright (c) 2009 Ingres Corporation. All rights reserved.
*/

# include   <compat.h>
# include   <gl.h>
# include   <clconfig.h>
# include   <systypes.h>
# include   <cs.h>
# include   <me.h>
# include   <meprivate.h>
# include   <qu.h>

/*
**  Name: METLS.C - Thread-local storage routines
**
**  Description:
**	This module contains the code which implements 
**	thread-local storage.
**
**
**  History:
**	25-mar-1997 (canor01)
**	    Created.
** 	23-Sep-2009 (wonst02) SIR 121201
** 	    Replace mutex with critical section - contention is within process.
** 	    Replace hex constant with TLS_OUT_OF_INDEXES.
*/

static bool		ME_tls_init = FALSE;
static CRITICAL_SECTION	ME_tls_critical_section;

GLOBALREF ME_TLS_KEY	ME_tlskeys[];

/*
** Name:        ME_tls_createkey    - create a thread-local storage index key
**
** Description:
**      Create a unique key to identify a particular index into
**      thread-local storage.
**
** Inputs:
**      None
**
** Outputs:
**	key		the key created
**	status		
**          OK  - success
**          !OK - failure
**
**  History:
**      25-mar-1997 (canor01)
**          Created.
** 	23-Sep-2009 (wonst02) SIR 121201
** 	    Replace mutex with critical section - contention is within process.
** 	    Replace hex constant with TLS_OUT_OF_INDEXES.
*/
VOID
ME_tls_createkey( ME_TLS_KEY *key, STATUS *status)
{
    ME_TLS_KEY		i;
    
    /* first time in, initialize critical section for TLS index array */
    if ( ME_tls_init == FALSE )
    {
	InitializeCriticalSection(&ME_tls_critical_section);	
	ME_tls_init = TRUE;
    }
    *key = TlsAlloc();
    if ( *key == TLS_OUT_OF_INDEXES )
    {
        *status = FAIL;
	return;
    }
    *status = OK;

    /*
    ** key was created successfully.
    ** now insert it into an array of active keys
    */
    EnterCriticalSection(&ME_tls_critical_section);
    for ( i = 0; i < ME_MAX_TLS_KEYS; i++ )
    {
	if ( ME_tlskeys[i] == *key )
	    break;
	if ( ME_tlskeys[i] == (ME_TLS_KEY)0 )
        {
	    ME_tlskeys[i] = *key;
	    break;
	}
    }
    LeaveCriticalSection(&ME_tls_critical_section);
}

/*
** Name:        ME_tls_destroyall     - Destroy a thread's local storage
**
** Description:
**      Destroy the thread-local storage associated with a
**      all keys for a particular thread.
**
** Inputs:
**      destructor - pointer to a function to de-allocate the memory.
**
** Outputs:
**
**      Returns:
**          OK  - success
**          !OK - failure
**
**  History:
**	25-mar-1997 (canor01)
**	    Created.
*/
VOID
ME_tls_destroyall( STATUS *status )
{
    ME_TLS_KEY	key;
    STATUS	ret;

    *status = OK;

    /* keys begin at 1, keytab array begins at zero */
    EnterCriticalSection(&ME_tls_critical_section);
    for ( key = 0; key < ME_MAX_TLS_KEYS; key++ )
    {
        if ( ME_tlskeys[key] == (ME_TLS_KEY)0 )
	    break;
	ME_tls_destroy( ME_tlskeys[key], &ret );
	if ( ret != OK )
	    *status = ret;
    }
    LeaveCriticalSection(&ME_tls_critical_section);

    return;
}

