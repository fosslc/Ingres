/*
**Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <pc.h>
#include <gl.h>
#include <sl.h>
#include <iicommon.h>
#include <dbdbms.h>
#include <duf.h>
#include <er.h>
#include <duerr.h>
#include <duucatdef.h>
#include <cs.h>
#include <lk.h>
#include <dudbms.h>
#include <dusdb.h>
#include <duenv.h>

/*
** Name:        dusdata.c
**
** Description: Global data for dus facility.
**
** History:
**
**      28-sep-96 (mcgem01)
**          Created.
*/

GLOBALDEF char  iiduNoFeClients[] = ERx("nofeclients"); /* special client name*/
GLOBALDEF DU_ENV              *Dus_dbenv ZERO_FILL;
GLOBALDEF DU_ERROR            *Dus_errcb ZERO_FILL;
GLOBALDEF DUU_CATDEF          *Dus_allcat_defs[DU_MAXCAT_DEFS] ZERO_FILL;
