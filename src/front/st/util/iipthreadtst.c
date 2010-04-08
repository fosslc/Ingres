/*
** Copyright (c) 2005 Ingres Corporation
*/

# include <compat.h>
# include <pc.h>
#ifdef LNX
# include <unistd.h>
#endif

/*
**  Name: iipthreadtst -- program used by iirundbms to avoid starting Ingres
**	built for the Linux NPTL pthread library on a machine that does not
**	support that library.
**
**  Usage:
**	iipthreadtst
**
**  Inputs:
**	none
**
**  Outputs:
**	if FAIL an English language error message is written to stdout
**
**  Returns:
**	PCexit ( OK )	if non-Linux or pthread library status is okay
**	PCexit ( FAIL )	if attempt to run NPTL on Linux Threads platform
**
**  History:
**	03-aug-2005 (toumi01)
**	    Created.
*/

/*
PROGRAM =	iipthreadtst
**
NEEDLIBS =	COMPATLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

int main()
{
#ifdef LNX
#ifndef SIMULATE_PROCESS_SHARED
    if ( sysconf(_SC_THREAD_PROCESS_SHARED) < 0 )
    {
	if (errno == EINVAL)
	    PCexit( OK );
	SIprintf("\n\nIngres build is not compatible with the OS POSIX threading library.\n");
	PCexit( FAIL );
    }
#endif
#endif
    PCexit( OK );
}
