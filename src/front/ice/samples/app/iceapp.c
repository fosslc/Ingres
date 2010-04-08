/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: iceapp.c
**
** Description:
**      Program displays a numbered list of arguments that it was invoked with.
**
**      The app_dir parameter in config.dat should be set to the path of the
**      executable.
**      e.g.
**          UNIX        /ingresii/ingres/ice/bin
**          Windows NT  c:\ingresii\ingres\ice\bin
**
PROGRAM = iceapp.cgi

DEST = icebin
##
## History:
##      07-Jun-2000 (fanra01)
##          Bug 101731
##          Created.
##
*/
# include <stdio.h>

int
main( int argc, char** argv )
{
    int retval = 0;
    int i;

    for (i=0; i < argc; i++)
    {
        printf( "%04d %s\n", i, argv[i] );
    }

    return( retval );
}
