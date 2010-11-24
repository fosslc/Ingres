/*
**Copyright (c) 2004 Ingres Corporation
*/
#include <compat.h>
#include <er.h>
#include <ep.h>
#include <gl.h>
#include    <cs.h>
#include <pc.h>
#include <st.h>
#include <tr.h>
#include <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include <lg.h>
#include <cv.h>
#include <cx.h>

/*
** Name: lgtpoint	- set a LG testpoint on or off
**
** Usage:
**	lgtpoint -N		- sets testpoint N on
**	lgtpoint -N -on		- sets testpoint N on
**	lgtpoint -N -off	- sets testpoint N off
**
**	If the RCP is not online, the caller is told to define II_LG_TPOINT
**	instead, since LG memory testpoints only persist if the RCP is online.
**
**	NOTE: THIS PROGRAM IS ONLY USED BY THE DUAL LOGGING TESTBED!!!
**
** History:
**	18-jul-1990 (bryanp)
**	    Created.
**	30-aug-1990 (bryanp)
**	    Show & display lgd_test_badblk if set.
**	23-oct-1990 (bryanp)
**	    Ported to Unix
**	17-nov-1992 (bryanp)
**	    Update mkming hints so this program will build correctly.
**	18-jan-1993 (bryanp)
**	    Tinker around some more with NEEDLIBS hints
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	18-oct-1993 (rmuth)
**	    CL prototype, add <cv.h>.
**	8-jun-94 (robf)
**          Add ADFLIB to the NEEDLIBS list.
**	24-feb-1997 (walro03)
**	    More tweaking of the NEEDLIBS list so lgtpoint will link.
**	24-nov-1997 (walro03)
**	    Move ADFLIB and ULFLIB to the end of NEEDLIBS so lgtpoint.will link.
**	20-feb-1998 (toumi01)
**	    A comment re Rob's (walro03) change to make lgtpoint link:
**	    as he remarked in his email about this change, there will STILL be
**	    two unresolved references: IIudadt_register and IIclsadt_register.
**	    To resolve these, do a "ming -n" to a file, and edit the file to
**	    make it a valid script with $ING_BUILD/lib/iiuseradt.o and 
**	    $ING_BUILD/lib/iiclsadt.o added to the list of objects to be
**	    included for lgtpoint.  This linking problem was seen on all the
**	    Ingres 2.0 ports, and was never fully resolved because lgtpoint
**	    "is only a test program".
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-may-2001 (devjo01)
**	    Add cx.h for CXenable_cluster(). s103715.
**	08-Jul-2004 (hanje04)
**	   Tweak NEEDLIBS to got lgtpoint to link.
**	28-Jul-2004 (hanje04)
**	   Remove references to SDLIB
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**	25-Jul-2007 (drivi01)
**	    Added routines to detect on Vista OS if user is running
**	    with elevated privileges.  This program can only be
**	    run with elevated privileges.
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Add notreached return(FAIL) to silence compiler warnings.
**	
[@history_template@]...
*/
/*
**      mkming hints
**
NEEDLIBS =      SCFLIB UDTLIB QEFLIB DMFLIB CUFLIB GCFLIB SXFLIB GWFLIB OPFLIB QSFLIB RDFLIB TPFLIB RQFLIB PSFLIB ADFLIB ULFLIB COMPATLIB MALLOCLIB

OWNER =         INGUSER

PROGRAM = lgtpoint

UNDEFS =        scf_call

MODE = SETUID
*/
i4
main(argc,argv)
i4	argc;
char	*argv[];
{
    CL_ERR_DESC	    sys_err;
    STATUS	    status;
    i4	    test_point;
    i4	    test_val;
    i4	    length;
    i4		    action;
#define			TURN_ON	    1
#define			TURN_OFF    2
    i4	    lgd_status;

    MEadvise(ME_INGRES_ALLOC);
    status = TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT, &sys_err);
    if (status)
	PCexit(status);

    /* 
    ** Check if this is Vista, and if user has sufficient privileges 
    ** to execute.
    */
    ELEVATION_REQUIRED();

    for (;;)
    {
	if (argc < 2 || argc > 3)
	{
	    TRdisplay("Usage: lgtpoint -<tpoint-number>[-on,-off]\n");
	    break;
	}
	if (LGinitialize(&sys_err, ERx("lgtpoint")) != OK)
	{
	    TRdisplay("Can't initialize logging system.\n");
	    TRdisplay("If RCP is down, run csinstall -\n");
	    break;
	}
	/*
	** Parse optional -on or -off
	*/
	if (argc == 3)
	{
	    if (STcasecmp(&argv[2][1], "ON" ) == 0)
		action = TURN_ON;
	    else if (STcasecmp(&argv[2][1], "OFF" ) == 0)
		action = TURN_OFF;
	    else
	    {
		TRdisplay("Can't interpret '%s' as either ON or OFF.\n",
			    &argv[2][1]);
		break;
	    }
	}
	else
	{
	    action = TURN_ON;
	}
	if (CVal(&(argv[1][1]), &test_point) != OK)
	{
	    TRdisplay("Can't interpret '%s' as a testpoint number.\n",
			argv[1]);
	    break;
	}
	status = LGshow(LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
                        &length, &sys_err);
	if (status)
	{
	    TRdisplay("Can't show logging system status.\n");
	    break;
	}
	if ((lgd_status & 1) == 0)
	{
	    TRdisplay("Logging system not ONLINE.\n");
	    TRdisplay("Define II_LG_TPOINT instead.\n");
	    break;
	}
	length = test_point;
	if (LGshow(LG_S_TP_VAL, (PTR)&test_val, sizeof(test_val), &length,
		    &sys_err) != OK)
	{
	    TRdisplay("Can't show previous testpoint value for %d\n",
			test_point);
	    break;
	}
	if (test_val != 0 && action == TURN_ON)
	    TRdisplay("lgtpoint: testpoint %d is already on\n", test_point);
	else if (test_val == 0 && action == TURN_OFF)
	    TRdisplay("lgtpoint: testpoint %d is already off\n", test_point);

	if (action == TURN_ON)
	{
	    if (LGalter(LG_A_TP_ON, (PTR)&test_point, sizeof(test_point),
			&sys_err))
	    {
		TRdisplay("Can't alter testpoint %d to ON.\n", test_point);
		break;
	    }
	}
	else
	{
	    if (LGalter(LG_A_TP_OFF, (PTR)&test_point, sizeof(test_point),
			&sys_err))
	    {
		TRdisplay("Can't alter testpoint %d to OFF.\n", test_point);
		break;
	    }
	}

	length = test_point;
	if (LGshow(LG_S_TP_VAL, (PTR)&test_val, sizeof(test_val), &length,
		    &sys_err) != OK)
	{
	    TRdisplay("Can't show updated testpoint value for %d\n",
			test_point);
	    break;
	}
	if ((test_val == 0 && action == TURN_ON) ||
	    (test_val != 0 && action == TURN_OFF))
	{
	    TRdisplay("ERROR! Testpoint %d was unsuccessfully altered.\n",
			test_point);
	    break;
	}
	/*
	** Show & display the selected badblk/badfil if they're set
	*/
	if (LGshow(LG_S_TEST_BADBLK, (PTR)&test_val, sizeof(test_val), &length,
		    &sys_err) != OK)
	{
	    TRdisplay("Can't show LG_S_TEST_BADBLK\n");
	    break;
	}
	if (test_val)
	    TRdisplay("Block %d has been selected to be 'bad'\n", test_val);
	else
	    TRdisplay("There is no 'bad' block\n");

	PCexit(OK);
    }

    PCexit(FAIL);
    
    /* NOTREACHED */
    return(FAIL);
}
