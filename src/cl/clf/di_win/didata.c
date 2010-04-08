/*
**      DI Data
**
**  Description
**      File added to hold all GLOBALDEFs in DI facility.
**      This is for use in DLLs only.
**
LIBRARY = IMPCOMPATLIBDATA
**
**  History:
**      12-Sep-95 (fanra01)
**          Created.
**	08-dec-1997 (canor01)
**	    Add DI_misc_sem.
**	28-jan-1998 (canor01)
**	    Add DI_lru_queue and DI_lru_initialized.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	05-nov-1999 (somsa01)
**	    Added Di_gather_io.
*/

#include    <compat.h>
#include    <cs.h>
#include    <tm.h>
#include    <qu.h>

GLOBALDEF TM_PERFSTAT	Di_moinfo ZERO_FILL;
GLOBALDEF i4 		Di_moinited = FALSE;

GLOBALDEF i4 Di_reads = 0;	/* used by TMperfstat */
GLOBALDEF i4 Di_writes = 0;	/* used by TMperfstat */

GLOBALDEF CS_SEMAPHORE  DI_misc_sem ZERO_FILL;

GLOBALDEF QUEUE   DI_lru_queue ZERO_FILL;
GLOBALDEF bool    DI_lru_initialized = FALSE;

GLOBALDEF i4      Di_gather_io = FALSE;
