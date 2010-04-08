/*
** Copyright (c) 2000 Ingres Corporation
*/

/*
** Name: installcf
**
** Description:
**      Update the ColdFusion extention file.
**
** History:
**      11-Feb-2000 (fanra01)
**          Created.
*/
#include <compat.h>

#include "cfinst.h"

/*
** Name: main
**
** Description:
**      Program entry for adding ICE ColdFusion extenstion tags
**
** Inputs:
**      argv[1]     ColdFusion extension tag directory
**      argv[2]     Ingres ColdFusion extension tag directory
**
**                  If either path is set then it is used in preference
**                  to the paths configured in the registry.
**
** Outputs:
**      None.
**
** Returns:
**      OK          Completed sucessfully
**      !OK         Failed
**
** History:
**      12-Feb-2000 (fanra01)
**          Created.
*/
STATUS
main( int argc, char** argv )
{
    STATUS  status = OK;
    char*   cfpath = NULL;
    char*   icfpath = NULL;

    switch (argc)
    {
        case 1:     /* no arguments use registry configured values */
            break;
        case 3:     /* override ice extension tags directory */
            icfpath =(argv[2] && *argv[2]) ? argv[2] : NULL;
        case 2:     /* override ColdFusion extension tag directory */
            cfpath = (argv[1] && *argv[1]) ? argv[1] : NULL;
            if ((*argv[1] == '-' || *argv[1] == '/') &&
                ((*(argv[1] + 1) == '?' || *(argv[1] + 1) == 'h')))
            {
                PRINTF( "Usage:\n\tinstallcf " );
                PRINTF( "[<ColdFusion extension path>] [<ICE extension path>]]" );
                status = FAIL;
            }
            break;
        default:
            PRINTF( "Usage:\n\t%s ", argv[0] );
            PRINTF( "[<ColdFusion extension path>] [<ICE extension path>]]" );
            status = FAIL;
            break;
    }

    if ((status == OK) &&
        (status = RegisterColdFusionICEExtensions( cfpath, icfpath )) != OK)
    {
        PRINTF( "Error: instcf failed with status %d\n", status );
    }
    else
    {
        PRINTF( "%s: Successfully updated %s.\n", argv[0], CFTAGS );
    }
    return (status);
}
