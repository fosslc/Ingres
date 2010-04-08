/*
** Copyright (C) 2000 Ingres Corporation
*/

/*
** Name: cfinst.c
**
** Description:
**      File contains the file for updating the extension files.
**
** History:
**      11-Feb-2000 (fanra01)
**          Created.
*/

# include <compat.h>

# include "cfinst.h"

static char* cfvtm  = { CFTAGS };
static char* icfvtm = { ICFTAGS };
static char* stag = { STRTTAG };
static char* etag = { ENDTAG };

/*
** Name: DropExistingTags
**
** Description:
**      Removes the tags between the begin and end tags inclusive.
**
** Inputs:
**      Start if the tags.
**
** Outputs:
**      Updated start of the tags.
**
** Returns:
**      OK      there is no error return.
*/
STATUS
DropExistingTags( PRL* begin )
{
    STATUS  retval = OK;
    PRL     begintag;
    PRL     endtag;

    if (((begintag = CFScanConf( stag, *begin, FALSE )) != NULL) &&
        ((endtag = CFScanConf( etag, *begin, FALSE )) != NULL))
    {
        if (begintag != NULL)
        {
            if (begintag == *begin)
            {
                *begin = endtag->next;
            }
            else
            {
                begintag->prev->next  = endtag->next;
            }
            endtag->next = NULL;
        }
        for (endtag = begintag; endtag != NULL; endtag = begintag)
        {
            begintag = begintag->next;
            HFREE( endtag->text );
            HFREE( (PTR)endtag );
        }
    }
    return (retval);
}

/*
** Name: MergeTags
**
** Description:
**      Prepends the Ingres markup extensions to the ColdFusion file.
**
** Inputs:
**      mbegin      Starting tag of the target
**      tbegin      Set of tags to prepend
**
** Outputs:
**      mbegin      The set of merged tags
**
** Returns:
**      OK          There is no error return.
*/
STATUS
MergeTags( PRL tbegin, PRL* mbegin )
{
    STATUS retval = OK;

    PRL tend = tbegin->prev;
    PRL mend = (*mbegin)->prev;

    tend->next = *mbegin;
    (*mbegin)->prev = tend;
    tbegin->prev = mend;
    *mbegin = tbegin;

    return (retval);
}

/*
** Name: RegisterColdFusionICEExtensions
**
** Description:
**      Backup the existing extension file, remove any existing tags and
**      prepend the ICE extensions to the file.
**
** Inputs:
**      cfpath      ColdFusion extension path
**      icfpath     Ice ColdFusion extension path
**
** Outputs:
**      None.
**
** History:
**      11-Feb-2000 (fanra01)
**          Created.
*/
STATUS
RegisterColdFusionICEExtensions( char* cfpath, char* icfpath )
{
    STATUS      status  = FAIL;
    PRL         cbegin  = NULL;
    PRL         ibegin  = NULL;
    int         version;
    LOCATION    cloc;
    LOCATION    iloc;
    char        clocpath[MAX_LOC + 1];
    char        ilocpath[MAX_LOC + 1];

    LOfroms( PATH, clocpath, &cloc );
    LOfroms( PATH, ilocpath, &iloc );

    cfpath = SetColdFusionPath( cfpath );
    icfpath = SetIngresICEPath( icfpath );

    /*
    ** Detect if ColdFusion is on the system
    */
    if ((cfpath != NULL) || ((version = IsColdFusionInstalled()) > 0))
    {
        do
        {
            /*
            ** Function parameters override the values determined from
            ** registry.
            */
            if (((icfpath != NULL) ||
                ((icfpath = GetIngresICECFPath()) != NULL)) &&
                ((cfpath != NULL) ||
                ((cfpath = GetColdFusionPath( version )) != NULL)))
            {
                if ((status = CFReadConf( icfpath, icfvtm, &iloc, &ibegin ))
                    != OK)
                    break;

                if ((status = CFReadConf( cfpath, cfvtm, &cloc, &cbegin ))
                    != OK)
                    break;

                if ((status = CFBackupConf( &cloc, cbegin )) != OK)
                    break;

                if ((status = DropExistingTags( &cbegin )) != OK)
                    break;

                if ((status = MergeTags( ibegin, &cbegin )) != OK)
                    break;

                status = CFWriteConf( &cloc, cbegin );
            }
            break;
        }
        while(TRUE);

        if(status != OK)
        {
            /*
            ** we had an error
            */
        }
    }
    else
    {
        /*
        ** Get ColdFusion from Allaire web site
        */
        status = FAIL;
    }
    return  (status);
}
