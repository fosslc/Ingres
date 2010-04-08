/*
**      TM Data
**
**  Description
**      File added to hold all GLOBALDEFs in TM facility.
**      This is for use in DLLs only.
**
LIBRARY = IMPCOMPATLIBDATA
**
**  History:
**      12-Sep-95 (fanra01)
**          Created.
**
*/
#include    <compat.h>

# ifdef DESKTOP
GLOBALDEF   ULONG       cs_start_msecs = 0;
# endif /* DESKTOP */
