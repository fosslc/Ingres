/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <me.h>
#include    <errno.h>
#include    <cs.h>
#include    <fdset.h>
#include    <csev.h>
#include    <cssminfo.h>
#include    <tr.h>

/**
**
**  Name: CSSHMEMTEST.C - A simple test of CS shared memory
**
**  Description:
**	This program installs CS shared resources like 'csinstall' but
**	also checks that the shared memory is really shared and working.
**
**          main() - shared memory test
**
**
**  History:
**      08-sep-88 (anton)
**          First version
**	11-Mar-89 (GordonW)
**	    add ming hints
**	5-june-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Add include <clconfig.h> to pick up correct ifdefs in <csev.h>;
**		Don't map the same segment twice for dr5_us5 and ib1_us5 as
**		they have a bug that does not allow the same segment to be
**		mapped twice in the same process.
**	3-jul-1992 (bryanp)
**	    Pass new num_users arg to CS_create_sys_segment().
**	04-feb-1993 (sweeney)
**	    Tack usl_us5 onto the dr5 and ib1 case - can't map same 
**	    segment twice.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      26-jul-1993 (mikem)
**          Include systypes.h now that csev.h exports prototypes that include
**          the fd_set structure.
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	13-mar-1995 (smiba01)
**	    Added code for dr6_ues.  These changes taken from ingres63p
**	    library (29-apr-93 (ajc)).
**      10-nov-1995 (murf)
**              Added sui_us5 to all areas specifically defined with usl_us5.
**              Reason, to port Solaris for Intel.
**	19-apr-1995 (canor01)
**		change errno to errnum for reentrancy
**	17-may-1998 (canor01)
**	    Eliminate requirement that mapping shared memory segments a
**	    second time in the same process yields different mapping addresses.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*
**  Forward and/or External function references.
*/

VOID perr();    			/* declaration of external proc */
static i4	exit_stat = OK;


/*
**  Defines of other constants.
*/

/*	testing constants */

#define			SERVER_SEG_SIZE	    1024
#define			NUM_OF_VALUES	    (SERVER_SEG_SIZE / sizeof(i4))


/*
PROGRAM = csshmemtest

MODE = SETUID

OWNER = INGUSER

NEEDLIBS = COMPAT MALLOCLIB
*/

/*{
** Name: main()	- test of CS shared memory
**
** Description:
**	See above description
**
** Inputs:
**	argc		argument count	- ignored
**	argv		argument vector - ignored
**
** Outputs:
**	none
**
**	Returns:
**	    PCexit()
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-sep-88 (anton)
**          First Version
**	3-jul-1992 (bryanp)
**	    Pass new num_users arg to CS_create_sys_segment().
*/
i4
main(argc, argv)
i4            argc;
char               *argv[];
{
    CL_ERR_DESC	err_code;
    CS_SMCNTRL	*cs_sm_cb, *cs2_sm_cb;
    PTR		server_segment, server2_segment;
    STATUS	status;
    i4	*i;
    u_i4	shmid;
    i4		count;
    u_i4	server_num;
    bool	serv = FALSE, sys = TRUE;

    MEadvise(ME_INGRES_ALLOC);

    TRset_file(TR_F_OPEN,"stdio",5,&err_code);

    /* create the segment */
    if ((status = CS_create_sys_segment(2, 50, &err_code)))
    {
	perr("WARNING: CS_create_sys_segment: could not create the system segment\n",
		status, errno);
	sys = FALSE;
	exit_stat = OK;
    }

    if (status = CS_map_sys_segment(&err_code))
    {
	perr("CS_map_sys_segment: could not map the system control block\n",
		status, errno);
	goto exit_point;
    }

    CS_get_cb_dbg(&cs_sm_cb);

    if (status = CS_map_sys_segment(&err_code))
    {
	perr("CS_map_sys_segment: could not map the system control block twice",
		status, errno);
	goto exit_point;
    }

    CS_get_cb_dbg(&cs2_sm_cb);

    if ((cs_sm_cb != cs2_sm_cb) &&
	(MEcmp((PTR) cs_sm_cb, (PTR) cs2_sm_cb, sizeof(CS_SMCNTRL))))
    {
	perr("2nd map of CS_SMCNTRL doesn't point to same data\n", status, errno);
	goto exit_point;
    }

    /* create the server segment */

    if (status = CS_alloc_serv_segment((u_i4) SERVER_SEG_SIZE, 
				       &server_num, &server_segment,&err_code))
    {
	perr("CS_alloc_serv_segment: could not create the server segment\n",
		status, errno);
	goto exit_point;
    }
    serv = TRUE;

    /* map it */
    /* set some values */

    i = (i4 *) server_segment;

    for (count = 0; count < NUM_OF_VALUES; count++)
    {
	*i++ = count;
    }

    /* set some values */
    /* map it again */
    if (status = CS_map_serv_segment(server_num, &server2_segment, &err_code))
    {
	perr("CS_map_serv_segment: could not map the server segment\n",
		status, errno);
	goto exit_point;
    }

    if ((server_segment != server2_segment) &&
	(MEcmp((PTR) server_segment, (PTR) server2_segment, 
					sizeof(i4) * NUM_OF_VALUES)))
    {
	perr("2 maps server segments doesn't match\n", status, errno);
	goto exit_point;
    }

    i = (i4 *) server2_segment;
    for (count = 0; count < NUM_OF_VALUES; count++)
    {
	if (*i++ != count)
	    break;
    }
    if (count != NUM_OF_VALUES)
    {
	perr("2nd map does not have right values\n", status, errno);
	goto exit_point;
    }

    /* destroy it */
exit_point:
    if(serv)
    {
    	if (status = CS_destroy_serv_segment(server_num, &err_code))
		perr("CS_destroy_serv_segment: could not destroy the server segment\n",
			status, errno);
    }

    if(sys)
    {
	if(status = CS_des_installation())
		perr("CS_des_installation: could not destroy the sys segment\n",
			status, errno);
    }

    PCexit(exit_stat);
}

VOID
perr(string, ret_val, errnum)
char	*string;
STATUS	ret_val;
int	errnum;
{
    SIprintf("%s status returned: 0x%x; errno returned: %d\n",
			string, ret_val, errnum);
    exit_stat = FAIL;
}
