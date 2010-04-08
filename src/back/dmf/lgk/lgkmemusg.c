
/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <cs.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <si.h>
#include    <tr.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dm.h>
#include    <lg.h>
#include    <lk.h>
#include    <lgkdef.h>
#include    <lgkparms.h>


/**
**
**  Name: LGKMEMUSG.C - Print out shared memory requirements of the LG/LK system
**
**  Description:
**  	Print out shared memory requirements of the LG/LK system.   See "main"
**	comments for more info.
**
**	USAGE for main program:
**		iishowres [-d]
**
**          main() 			- the main program.
**
**  History:
**      18-feb-90 (mikem)
**          created.
**	21-mar-90 (mikem)
**	    Added extra error handling to deal with an "rcp.par" file with a bad
**	    format.  Now will PCexit(FAIL) if there are not enough lines in the
**	    rcp.par file, or if any of the fields we are interested in can't be
**	    converted from string to integer.
**	13-jan-1992 (bryanp)
**	    Added support for dual logging (one more "y/n" line at the end of
**	    a 6.5 rcp.par). We don't use this line for anything, but it's
**	    important to keep this file up to date.
**	11-feb-1993 (keving)
**	    Removed usage of rcp.par file. Now we read resource sizes from
**	    PM. Use sizing functions provided by lgk.
**	29-apr-1993 (bryanp)
**	    Convert to a dmfmerge executable for better linking on Unix.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-apr-2001 (devjo01)
**	    Change parameters to lgk_calculate_size, and add call to
**	    lgk_get_counts (s103715).
**	30-Oct-2002 (hanch04)
**	    Changed size to SIZE_TYPE to handle large memory
**/

/*
** Allows NS&E to build one executable for IIDBMS, DMFACP, DMFRCP, ...
*/
# ifdef II_DMF_MERGE
# define    MAIN(argc, argv)    iishowres_libmain(argc, argv)
# else
# define    MAIN(argc, argv)    main(argc, argv)
# endif

/*
**      mkming hints
MODE =          SETUID PARTIAL_LD

NEEDLIBS =      SCFLIB DMFLIB ADFLIB ULFLIB GCFLIB COMPATLIB MALLOCLIB

OWNER =         INGUSER

PROGRAM =       iishowres
*/

FUNC_EXTERN VOID LG_size();    		/* return size of LG object */
FUNC_EXTERN VOID LK_size();    		/* return size of LK object */
static STATUS	_iishowres(i4 argc,char *argv[]);


/*{
** Name: main()	- main routine of iishowmem
**
** Description:
**	Main routine of iishowmem.
**
**	This program is called by the installation scripts as part of the 
**	"iibuild" procedures.  It reads the PM parameters 
**	to determine the LG/LK parameters the user has chosen.  From 
**	this it calculates the MAXIMUM shared memory requirements of the LG/LK 
**	system.  Given no arguments this program will simply print out a single
**	number which is the value which II_LGLK_MEM should be set to.  If the 
**	"-d" option is specified it will dump out a more "wordy" description of
**	the memory requirements.
**
**	USAGE for main program:
**		iishowres [-d]
**
**	Example output when run with no arguments:
**
**	    dryad:mikem 201> iishowres
**	    630784
**
**	Example output when run with the "-d" argument:
**
**	    dryad:mikem 202> iishowres -d
**	    Memory allocation for 512 lock lists = 45056 bytes
**	    Memory allocation for 3000 locks = 228000 bytes
**	    Memory allocation for hash tables (257 lock hash table,257 
**		resource hash table) = 2056 bytes.
**	    Memory allocation for 4 (16 K) log buffers = 65536 bytes
**	    Memory allocation for 512 transactions = 131072 bytes
**	    Memory allocation for 512 databases = 116736 bytes
**	    Memory allocation for internal data structures = 42328 bytes
**	    Total allocation for logging/locking resources = 630784 bytes
**
** Inputs:
**      argc                         	number of arguments.
**      argv                            "-d" or nothing.
**
** Outputs:
**	none.
**
**	Returns:
**	    exits to the OS with either OK or FAIL.
**
** History:
**      18-feb-90 (mikem)
**          created.
**	18-apr-2001 (devjo01)
**	    Change parameters to lgk_calculate_size, and add call to
**	    lgk_get_counts (s103715).
**	29-sep-2002 (devjo01)
**	    Call MEadvise only if not II_DMF_MERGE.
**	17-aug-2004 (thaju02)
**	    Changed SIprintf format from '%ld' to '%lu'. 
*/

VOID
# ifdef	II_DMF_MERGE
MAIN(argc, argv)
# else
main(argc, argv)
# endif
i4                  argc;
char		    *argv[];
{
# ifndef	II_DMF_MERGE
    MEadvise(ME_INGRES_ALLOC);
# endif
    PCexit(_iishowres(argc, argv));
}

static STATUS
_iishowres(
i4                  argc,
char		    *argv[])
{
    i4		wordy = FALSE;
    SIZE_TYPE		size;
    LGLK_INFO		lgkcount;
    CL_ERR_DESC		err_code;

    TRset_file(TR_F_OPEN, TR_OUTPUT, TR_L_OUTPUT, &err_code);

    if (argc > 2)
    {
	SIprintf("USAGE: iishowres [-d]\n");
	return(FAIL);
    }
    else if (argc == 2)
    {
	if ((argv[1][0] == '-') && (argv[1][1] == 'd'))
	{
	    wordy = TRUE;
	}
	else
	{
	    SIprintf("USAGE: iishowres [-d]\n");
	    return(FAIL);
	}
    }

    if (lgk_get_counts(&lgkcount, wordy))
	return(FAIL);

    if (lgk_calculate_size(wordy, &lgkcount, &size))
	return(FAIL);

    if (!wordy)
	SIprintf("%lu\n", size);

    return(OK);
}
