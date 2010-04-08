/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<duucatdef.h>

/*
** Name:        duidata.c
**
** Description: Global data for dui facility.
**
** History:
**      28-sep-96 (mcgem01)
**          Created.
*/

/* duifedef.c */

GLOBALDEF   char        *Dui_50scicat_commands[]        =
{
    "iidbcapabilities"
    /* No modify command for iidbcapabilities. */
};

GLOBALDEF   DUU_CATDEF  Dui_51scicat_defs[DU_5MAXSCI_CATDEFS + 1] ZERO_FILL;

