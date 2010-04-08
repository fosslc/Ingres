/*
**Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <pc.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <dbdbms.h>
# include <duf.h>
# include <cs.h>
# include <lk.h>
# include <er.h>
# include <duerr.h>
# include <dudbms.h>
# include <duenv.h>
# include <duddb.h>

/*
** Name:        duddata.c
**
** Description: Global data for dud facility.
**
** History:
**
**      28-sep-96 (mcgem01)
**          Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* dudestry.qsc */
GLOBALDEF DU_ENV              *Dud_dbenv ZERO_FILL;
GLOBALDEF DUD_MODE_VECT       *Dud_modes ZERO_FILL;
GLOBALDEF DU_ERROR            *Dud_errcb ZERO_FILL;
GLOBALDEF i4                  Dud_deadlock ZERO_FILL;
GLOBALDEF i4                  Dud_dlock_cnt ZERO_FILL;
GLOBALDEF i4                  Dud_1dlock_retry ZERO_FILL;
GLOBALDEF i4                  Dud_star_status ZERO_FILL;
GLOBALDEF char                Dud_appl_flag[DU_APPL_FLAG_MAX] ZERO_FILL;
