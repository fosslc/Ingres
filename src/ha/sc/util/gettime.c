/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

/*
**  Name:
**       gettime --
**
**  Usage:
**	The gettime program returns the value  of  time  in  seconds
**	since 00:00:00 UTC, January 1, 1970.
**
**  Description:
**      This program is designed to work in the SUN HA Cluster.
**
** 	This utility program, used by the probe method of the data service,
**	tracks the elapsed time in seconds from a known reference point 
**	(epoch point). It must be compiled and placed in the same directory 
**	as the data service callback methods (RT_basedir).
**
**  Exit Value:
**       0       the requested operation succeeded.
**       1       the requested operation failed.
**
** History:
**      01-Jan-2004 (hanch04)
**          Created for SUN HA cluster support.
**	13-Aug-2007 (bonro01)
**	    Fix compile problem with undef flag.
*/
/*
PROGRAM =       gettime
**
NEEDLIBS =      
**
UNDEFS =
**
DEST =          schabin
*/


int
main()
{
    printf("%d\n", time(0));
    exit(0);
}
