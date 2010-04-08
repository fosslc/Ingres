/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: iceref.c
**
** Description:
**      Module defines the entrypoints to map ICECTX structures to a handle.
**      This is to prevent the application from referencing the API structures
**      directly.
**
**      ic_getfreeentry     Take a created ICECTX structure and store it
**                          returning the index to locate it.
**      ic_getentry         Get the ICECTX structure from the handle.
**      ic_freeentry        Remove the ICECTX structure from its storage
**                          location.
**
** History:
**      21-Feb-2001 (fanra01)
**          Sir 103813
**          Created.
*/
# include <iiapi.h>
# include <iceconf.h>

# include "icelocal.h"

static ICECTXMAP icehandles = { 1, {0, NULL} };

/*
** Name: ic_getfreeentry
**
** Description:
**      Take a created ICECTX structure and store it returning the index
**      to locate it.
**
** Inputs:
**      picectx         Pointer to the ICECTX structure to store.
**
** Outputs:
**      hicectx         Pointer to the handle reference.
**
** Returns:
**      OK              Completed successfully.
**      FAIL
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_getfreeentry( PICECTX picectx, HICECTX* hicectx )
{
    ICE_STATUS status = OK;
    i4  i;
    i4  j;
    bool found = FALSE;

    for(i=0; (found == FALSE) && (i < (sizeof(i4) * BITSPERBYTE)); i+=1)
    {
        if (icehandles.inuse & (1<<i))
        {
            if (icehandles.row[i].used ^ 0xFFFFFFFF)
            {
                for(j=0; j < (sizeof(i4) * BITSPERBYTE); j+=1)
                {
                    if ((icehandles.row[i].used & (1<<j)) == 0)
                    {
                        found = TRUE;
                        break;
                    }
                }
                if (found == TRUE)
                {
                    break;
                }
            }
        }
        else
        {
            j = 0;
            icehandles.inuse |= (1<<i);
            icehandles.row[i].used |= 1;
            found = TRUE;
            break;
        }
    }
    if (found == TRUE)
    {
        if (icehandles.row[i].col[j] == NULL)
        {
            icehandles.row[i].used |= (1<<j);
            icehandles.row[i].col[j] = picectx;
            *hicectx = (i << (sizeof(i2) * BITSPERBYTE)) | j;
        }
    }
    else
    {
        status = FAIL;
    }
    return(status);
}

/*
** Name: ic_getentry
**
** Description:
**      Get the ICECTX structure from the handle.
**
** Inputs:
**      icectx
**
** Outputs:
**      picectx
**
** Returns:
**      OK              Completed successfully.
**      FAIL            Entry does not contain an ICECTX pointer.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_getentry( HICECTX icectx, PICECTX* picectx )
{
    ICE_STATUS status = OK;
    i4 row = (icectx >> (sizeof(i2) * BITSPERBYTE)) & 0xFFFF;
    i4 col = (icectx & 0xFFFF);

    if ((*picectx = icehandles.row[row].col[col]) == NULL)
    {
        status = FAIL;
    }
    return(status);
}

/*
** Name: ic_freeentry
**
** Description:
**      Remove the ICECTX structure from its storage location.
**
** Inputs:
**      icectx
**
** Outputs:
**      None.
**
** Returns:
**      OK              Completed successfully.
**      FAIL            Entry does not contain an ICECTX pointer.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_freeentry( HICECTX icectx )
{
    ICE_STATUS status = OK;
    i4 row = (icectx >> (sizeof(i2) * BITSPERBYTE)) & 0xFFFF;
    i4 col = (icectx & 0xFFFF);

    if (icehandles.row[row].col[col] == NULL)
    {
        status = FAIL;
    }
    else
    {
        icehandles.row[row].used &= ~(1<<col);
        icehandles.row[row].col[col] = NULL;
        if ((icehandles.row[row].used ^ 0xFFFFFFFF) == 0xFFFFFFFF)
        {
            icehandles.inuse &= ~(1<<row);
        }
    }
    return(status);
}
