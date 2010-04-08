/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <nm.h>
#include    <cs.h>
#include    <pc.h>
#include    <tr.h>
#include    <me.h>
#include    <cm.h>
#include    <er.h>
#include    <lo.h>
#include    <sl.h>
#include    <cv.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <scf.h>
#include    <dmf.h>
#include    <lk.h>
#include    <lg.h>

/* Ming hints
NEEDLIBS =	SCFLIB UDTLIB QEFLIB DMFLIB CUFLIB OPFLIB \
		PSFLIB RDFLIB ULFLIB GCFLIB SXFLIB GWFLIB \
		QSFLIB ADFLIB TPFLIB RQFLIB COMPATLIB 
*/

static STATUS	lock_event_tests(VOID);
static STATUS get_input(i4	    *data);


VOID 
main(argc, argv)
i4	argc;
char	*argv[];
{
    MEadvise(ME_INGRES_ALLOC);
    PCexit(lock_event_tests());
}

/*{
** Name: {@func_name@}	- {@comment_text@}
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      13-jul-93 (ed)
**          unnest dbms.h
**      02-apr-1997 (hanch04)
**          move main to front of line for mkmingnt
**	21-nov-1997 (walro03)
**	    Add NEEDLIBS so lkevtst will link.
**	20-feb-1998 (toumi01)
**	    Reverse ADFLIB and RDFLIB in NEEDLIBS to fix link.
**	    Note that as with back/dmf/lg/lgtpoint,c, there will STILL be
**	    two unresolved references: IIudadt_register and IIclsadt_register.
**	    To resolve these, do a "ming -n" to a file, and edit the file to
**	    make it a valid script with $ING_BUILD/lib/iiuseradt.o and 
**	    $ING_BUILD/lib/iiclsadt.o added to the list of objects to be
**	    included for lkevtst.
**	20-Jul-1998 (jenjo02)
**	    Lock event type and value changed from u_i4s to 
**	    LK_EVENT structure.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**    08-Jul-2004 (hanje04)
**       Tweak NEEDLIBS to got lgtpoint to link.
**    28-Jul-2004 (hanje04)
** 	 Remove references to SDLIB
**    29-Sep-2004 (drivi01)
**	 Updated NEEDLIBS, removed MALLOCLIB
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**	09-Dec-2008 (jonj)
**	    SIR 120874: replace uses of CL_SYS_ERR with CL_ERR_DESC
[@history_template@]...
*/
static STATUS
lock_event_tests(VOID)
{
    CL_ERR_DESC		sys_err;
    CL_ERR_DESC         cl_err;
    STATUS		status = FAIL;
    STATUS              ret_status;
    char		*env = 0;
    char		chname[MAX_LOC];
    LK_LLID		lock_list;
    LK_EVENT		lk_event;
    i4		operation;
    SCF_CB		scf_cb;

    status = TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT, &sys_err);
    if (status)
	return(FAIL);
    status = TRset_file(TR_I_OPEN, TR_INPUT,  TR_L_INPUT, &sys_err);
    if (status != OK)
	return (FAIL);

    CSinitiate(0, 0, 0);

    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_scm.scm_functions = 0;
    scf_cb.scf_scm.scm_in_pages = ((42 + SCU_MPAGESIZE - 1)
	& ~(SCU_MPAGESIZE - 1)) / SCU_MPAGESIZE;

    status = scf_call(SCU_MALLOC, &scf_cb);

    if (status != OK)
    {
	TRdisplay("scf_call failed\n");
	return (FAIL);
    }

    if (status)
    {
	TRdisplay("LOCKSTAT: failed to open trace file\n");
	return (FAIL);
    }
    else if (status = LKinitialize(&sys_err, ERx("lockstat")))
    {
	TRdisplay("LOCKSTAT: call to LKinitialize failed\n");
	TRdisplay("\tPlease check your installation and permissions\n");
	return (FAIL);
    }

    lk_event.type_high = 4;
    lk_event.type_low  = 0;
    lk_event.value = 91;

    do
    {
	if (get_input(&operation))
	    break;

	/*
	**  "1:clist 2:e_set 3:e_wait 4:e_clr 5:rlist 6:quit:>");
	*/

	switch(operation)
	{
	    case 1:
		status = LKcreate_list(
		    LK_ASSIGN | LK_NONPROTECT | LK_NOINTERRUPT,
		    (i4)0, (LK_UNIQUE *)0,
		    (LK_LLID *)&lock_list, 0,
		    &sys_err);
		if (status != OK)
		{
		    TRdisplay("Status %x from LKcreate_list, quitting\n",
				status);
		    operation = 6;
		}
		break;

	    case 2:
		status = LKevent(LK_E_SET | LK_E_CROSS_PROCESS,
		    lock_list, &lk_event, &sys_err);
		if (status != OK)
		{
		    TRdisplay("Status %x from LK_E_SET, quitting\n",
				status);
		    operation = 6;
		}
		break;

	    case 3:
		status = LKevent(LK_E_WAIT | LK_E_CROSS_PROCESS, lock_list,
				    &lk_event, &sys_err);
		if (status != OK)
		{
		    TRdisplay("Status %x from LK_E_WAIT, quitting\n",
				status);
		    operation = 6;
		}
		break;

	    case 4:
		status = LKevent(LK_E_CLR | LK_E_CROSS_PROCESS,
			lock_list, &lk_event, &sys_err);
		if (status != OK)
		{
		    TRdisplay("Status %x from LK_E_CLR, quitting\n",
				status);
		    operation = 6;
		}
		break;

	    case 5:
		status = LKrelease(LK_ALL, lock_list, (LK_LKID *)0,
			(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
		if (status != OK)
		{
		    TRdisplay("Status %x from LKrelease, quitting\n",
				status);
		    operation = 6;
		}
		break;

	    case 6:
		break;
	}
    }
    while (operation != 6);

    return (status);
}

static STATUS
get_input(
i4	    *data)
{
    STATUS		status = OK;
    char		buffer[12];
    i4		amount_read;
    CL_ERR_DESC		sys_err;
 
    while (1)
    {
	TRdisplay("1:clist 2:e_set 3:e_wait 4:e_clr 5:rlist 6:quit\n");
	MEfill(12, '\0', buffer);
	status = TRrequest(buffer, sizeof(buffer), &amount_read, 
	    &sys_err, 
	    "1:clist 2:e_set 3:e_wait 4:e_clr 5:rlist 6:quit:>");
	if (status != OK)
	    break;

	if (CVal(buffer, data) == OK && (*data >= 0) )
	    break;

	TRdisplay("Illegal data entry: '%s'. Enter the data again\n", buffer);
    }
    return (status);
}
