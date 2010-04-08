/*
**  Copyright (c) 1995, 2004 Ingres Corporation
*/

/*
**  Name: ME Data
**
**  Description
**      File added to hold all GLOBALDEFs in ME facility.
**      This is for use in DLLs only.
**
LIBRARY = IMPCOMPATLIBDATA
**
**  History:
**      12-Sep-95 (fanra01)
**          Created.
**	07-dec-2000 (somsa01)
**	    Added GLOBALDEF of MEmax_alloc.
**	08-feb-2001 (somsa01)
**	    Modified types of i_meactual and i_meuser to be SIZE_TYPE.
**	05-jun-2002 (somsa01)
**	    Added MEtaglist_mutex to synch up with UNIX.
**	22-jul-2004 (somsa01)
**	    Removed unnecessary include of erglf.h.
*/
#include <compat.h>
#include <clconfig.h>
#include <meprivate.h>
#include <me.h>
#include <cs.h>
#include <er.h>
#include <lo.h>
#include <mo.h>

GLOBALDEF SIZE_TYPE i_meactual = 0;
GLOBALDEF SIZE_TYPE i_meuser = 0;

/*
**  Data from meaddrmg
*/

GLOBALDEF QUEUE ME_segpool ZERO_FILL;


/*
**  Data from melifo
*/

GLOBALDEF STATUS ME_b_Status = 0;


/*
**  Data from mepages
*/

GLOBALDEF HANDLE ME_init_mutex = NULL;
GLOBALDEF u_char *MEalloctab = NULL;
GLOBALDEF char   *MEbase = NULL;
GLOBALDEF i4 MEmax_alloc = ME_MAX_ALLOC;

/*
**  Data from metls
*/

GLOBALDEF ME_TLS_KEY	ME_tlskeys[ ME_MAX_TLS_KEYS ] ZERO_FILL;

/*
**  Data used by metagutl
*/

GLOBALDEF CS_SYNCH	MEtaglist_mutex ZERO_FILL;
