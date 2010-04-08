/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <dbms.h>
#include    <er.h>
#include    <duerr.h>
#include    <me.h>
#include    <si.h>
#include    <st.h>
#include    <lo.h>
#include    <fe.h>
#include    <pc.h>
#include    <ug.h>
EXEC SQL INCLUDE 'dut.sh';
EXEC SQL INCLUDE SQLCA;

/**
**
**  Name: DUTINIT.SC - Initialization routines for StarView.
**
**  Description:
**
**          dut_ii1_init - Initialize StarView utility.
**	    dut_ii2_check_str - check string for valid DBname, Nodename...etc.
**	    dut_ii3_form_init - initialize forms for FRS.
**	    dut_ii4_chk_server- ** Removed **
**	    dut_ii5_init_helpfile - initialize the help file names into a
**				DUT_HELPCB structure.
**
**
**  History:
**      24-oct-1988 (alexc)
**          Initial Creation.
**	08-mar-1989 (alexc)
**	    Alpha test version.
**	06-apr-1989 (alexc)
**	    Change dut_ii5_init_helpfile() to use IIUGhlpname() to obtain
**		full path to help files(done for UNIX port).
**	24-Jul-1989 (alexc)
**	    remove function dut_ii4_chk_server().
**	01-Aug-1989 (alexc)
**	    change addform to IIUFgtGetForm(IIUFlcfLocateForm(), *formname)
**          in function dut_ii3_form_init.  This is for upholding the FE
**          standard of using form indexes for FE utilities.
**	02-jul-1991 (rudyw)
**	    Create a line of forward function references to remedy compiler
**	    complaint of redefinition.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of ug.h, fe.h, pc.h to eliminate gcc 4.3 warnings.
**/

/* forward_function_references */
DUT_STATUS dut_ii5_init_helpfile(), dut_ii2_check_str();


/*
** Name: dut_ii1_init	- Initialize StarView, load DDBname and Nodename
**				to DUT_CB.
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      24-oct-1988 (alexc)
**          Initial Creation.
*/
DUT_STATUS
dut_ii1_init(argc, argv, dut_cb, dut_errcb)
    int	    argc;
    char    **argv;
    DUT_CB	*dut_cb;
    DUT_ERRCB	*dut_errcb;
{
    DUT_STATUS	dut_status;
    DU_ERROR	*dut_error;

    MEadvise(ME_INGRES_ALLOC);
    Dut_gcb = dut_cb;
    Dut_gerrcb = dut_errcb;
    dut_errcb->dut_e9_flags = DU_SAVEMSG;
    dut_cb->dut_curr_iidbdbnode[0] = EOS;
    dut_cb->dut_c5_cdbname[0] = EOS;
    dut_cb->dut_c6_cdbnode[0] = EOS;
    dut_cb->dut_c8_prev_ldbnode[0] = EOS;
    dut_cb->dut_c9_prev_ldbname[0] = EOS;
    dut_cb->dut_c7_status   = DUT_S_NULL;
    dut_cb->dut_c8_status   = E_DU_OK;
    dut_cb->dut_c9_status   = DUT_F_FORM_1;
    dut_cb->dut_c10_prev_ddbnode[0] = EOS;
    dut_cb->dut_c11_prev_ddbname[0] = EOS;
    dut_cb->dut_c13_vnodename[0] = EOS;
    dut_cb->dut_c15_username[0] = EOS;
    dut_cb->dut_helpcb = NULL;
    dut_cb->dut_on_error_exit = FALSE;
    dut_cb->dut_on_error_noop = FALSE;
    dut_errcb->dut_cb = dut_cb;
    dut_ii5_init_helpfile(dut_cb, dut_errcb);
    if (argc == 1)
    {
	/* Means no parameter to StarView */

	dut_cb->dut_c1_ddbname[0] = EOS;
	dut_uu16_get_vnode(dut_cb, dut_errcb);
	STcopy(dut_cb->dut_c13_vnodename, dut_cb->dut_c2_nodename);
	return(E_DU_OK);
    }
    else if (argc == 2)
    {
	char    dut_arg1[DB_2MAXNAME];
	char	*dut_nodename = NULL;
	char	*dut_ddbname = NULL;
	char	*dut_ddbstar = NULL;

	/* Means there is a DDBname in the parameter list */

	STcopy( *(++argv), dut_arg1);
	dut_ddbname = STindex(dut_arg1, "::", 0);
	dut_ddbstar = STindex(dut_arg1, "/", 0);
	if (dut_ddbstar == NULL)
	{
	    /* No check necessary for this */
	}
	else if (!STcompare(dut_ddbstar, "/star"))
	{
	    *dut_ddbstar = EOS;
	}
	else if (!STcompare(dut_ddbstar, "/d"))
	{
	    *dut_ddbstar = EOS;
	}
	else
	{
	    /* Bad database name */
	    dut_ee1_error(dut_errcb, E_DU5112_SV_BAD_START_NAME, 0);
	    PCexit(0);
	}
	if (dut_ddbname == NULL || *dut_ddbname != ':')
	{
	    dut_ddbname = dut_arg1;
	    dut_nodename = dut_cb->dut_c2_nodename;
	    dut_cb->dut_c2_nodename[0] = EOS;
	}
	else if (*dut_ddbname == ':' && *(dut_ddbname + 1) == ':')
	{
	    *dut_ddbname++ = EOS;
	    dut_ddbname++;
	    dut_nodename = dut_arg1;
	}
	if (dut_nodename == NULL || *dut_nodename == EOS)
	{
	    dut_uu16_get_vnode(dut_cb, dut_errcb);
	    STcopy(dut_cb->dut_c13_vnodename, dut_cb->dut_c2_nodename);
	    dut_nodename = dut_cb->dut_c13_vnodename;
	}
	else
	{
	    if (dut_ii2_check_str(DUT_CK_NODENAME, dut_nodename, dut_errcb) 
		== W_DU1802_SV_BAD_NODENAME)
	    {
		dut_ee1_error(dut_errcb, W_DU1802_SV_BAD_NODENAME, 2,
		    0, dut_nodename);
	    }
	    STcopy(dut_nodename, dut_cb->dut_c2_nodename);
	}
	if (dut_ii2_check_str(DUT_CK_DDBNAME, dut_ddbname, dut_errcb)
	    == W_DU1803_SV_BAD_DDBNAME)
	{
	    dut_ee1_error(dut_errcb, W_DU1803_SV_BAD_DDBNAME, 2,
		0, dut_ddbname);
	}
	STcopy(dut_ddbname, dut_cb->dut_c1_ddbname);
    }
    return(E_DU_OK);
}


/*
** Name: dut_ii2_check_str	- check string for DDBname and Nodename.
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
*/
DUT_STATUS
dut_ii2_check_str(dut_flag, dut_name, dut_errcb)
    DUT_FLAG	dut_flag;
    char	*dut_name;
    DUT_ERRCB	*dut_errcb;
{
    switch(dut_flag)
    {
	case DUT_CK_NODENAME:
	    if (dut_uu5_chkdbname(dut_name, dut_errcb) != E_DU_OK)
	    {
		return(W_DU1802_SV_BAD_NODENAME);
	    }
	    break;
	case DUT_CK_DDBNAME:
	    if (dut_uu5_chkdbname(dut_name, dut_errcb) != E_DU_OK)
	    {
		return(W_DU1803_SV_BAD_DDBNAME);
	    }
	    break;
	default:
	    return(DUT_ERROR);
	    break;
    }
    return(E_DU_OK);
}


/*
** Name: dut_ii3_form_init	- initialize forms system
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      28-nov-1988 (alexc)
**          Creation.
*/
DUT_STATUS
dut_ii3_form_init(dut_cb, dut_errcb)
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
{
    PTR		pointer;
    FUNC_EXTERN STATUS   IIUFgtfGetForm();
    FUNC_EXTERN LOCATION *IIUFlcfLocateForm();

    EXEC FRS forms;
    dut_f1_intro 	= (FORM) "dut_f1_intro";
    dut_f2_ldblist	= (FORM) "dut_f2_ldblist";
    dut_f3_objlist	= (FORM) "dut_f3_objlist";
    dut_p2_select	= (FORM) "dut_p2_select";
    dut_p3_register	= (FORM) "dut_p3_register";
    dut_p6_nodelist	= (FORM) "dut_p6_nodelist";
    dut_p7_dbattr	= (FORM) "dut_p7_dbattr";
    dut_p7_1dbname	= (FORM) "dut_p7_1dbname";
    dut_p8_objattr	= (FORM) "dut_p8_objattr";
    dut_p9_viewattr	= (FORM) "dut_p9_viewattr";
    dut_p11_perror	= (FORM) "dut_p11_perror";
    dut_p12_ldblist	= (FORM) "dut_p12_ldblist";
    dut_p13_ownlist	= (FORM) "dut_p13_ownlist";
    dut_p14_node	= (FORM) "dut_p14_node";
    dut_p15_ldb		= (FORM) "dut_p15_ldb";
    dut_p16_table	= (FORM) "dut_p16_table";
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_f1_intro);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_f2_ldblist);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_f3_objlist);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_p2_select);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_p3_register);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_p6_nodelist);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_p7_dbattr);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_p7_1dbname);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_p8_objattr);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_p9_viewattr);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_p11_perror);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_p12_ldblist);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_p13_ownlist);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_p14_node);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_p15_ldb);
    IIUFgtfGetForm(IIUFlcfLocateForm(), (FORM) dut_p16_table);

    dut_errcb->dut_e10_form_init = TRUE;
    /* initialize form 1 */
    dut_cb->dut_form_intro				
	= (DUT_F1_INTRO *)MEreqmem(0, sizeof(DUT_F1_INTRO), TRUE, NULL);
    dut_cb->dut_form_intro->dut_f1_4ddblist		
	= (DUT_T1_DDBLIST *)NULL;
    dut_cb->dut_form_intro->dut_f1_4ddblist_curr_p	
	= (DUT_T1_DDBLIST *)MEreqmem(0, sizeof(DUT_T1_DDBLIST), TRUE, NULL);
    dut_cb->dut_form_intro->dut_f1_0ddb_size		
	= 0;
    dut_cb->dut_form_intro->dut_f1_0ddb_num_entry	
	= 0;

    /* initialize form 2 */
    dut_cb->dut_form_ldblist				
	= (DUT_F2_LDBLIST *)MEreqmem(0, sizeof(DUT_F2_LDBLIST), TRUE, NULL);
    dut_cb->dut_form_ldblist->dut_f2_3ldblist		
	= (DUT_T2_LDBLIST *)NULL;
    dut_cb->dut_form_ldblist->dut_f2_3ldblist_curr_p	
	= (DUT_T2_LDBLIST *)MEreqmem(0, sizeof(DUT_T2_LDBLIST), TRUE, NULL);
    dut_cb->dut_form_ldblist->dut_f2_0ldb_size		
	= 0;
    dut_cb->dut_form_ldblist->dut_f2_0ldb_num_entry	
	= 0;

    /* initialize form 3 */
    dut_cb->dut_form_objlist				
	= (DUT_F3_OBJLIST *)MEreqmem(0, sizeof(DUT_F3_OBJLIST), TRUE, NULL);
    dut_cb->dut_form_objlist->dut_f3_9objlist		
	= (DUT_T3_OBJLIST *)NULL;
    dut_cb->dut_form_objlist->dut_f3_9objlist_curr_p	
	= (DUT_T3_OBJLIST *)MEreqmem(0, sizeof(DUT_T3_OBJLIST), TRUE, NULL);
    dut_cb->dut_form_objlist->dut_f3_0obj_size		
	= 0;
    dut_cb->dut_form_objlist->dut_f3_0obj_num_entry	
	= 0;
    
    /* initialize popup 2 */
    dut_cb->dut_popup_select				
	= (DUT_P2_SELECT *)MEreqmem(0, sizeof(DUT_P2_SELECT), TRUE, NULL);

    /* initialize popup 3 */
    dut_cb->dut_popup_register				
	= (DUT_P3_REGISTER *)MEreqmem(0, sizeof(DUT_P3_REGISTER), TRUE, NULL);

    /* initialize popup 6 */
    dut_cb->dut_popup_nodelist				
	= (DUT_P6_NODELIST *)MEreqmem(0, sizeof(DUT_P6_NODELIST), TRUE, NULL);
    dut_cb->dut_popup_nodelist->dut_p6_1nodelist	
	= (DUT_T6_NODELIST *)NULL;
    dut_cb->dut_popup_nodelist->dut_p6_1nodelist_curr_p	
	= (DUT_T6_NODELIST *)MEreqmem(0, sizeof(DUT_T6_NODELIST), TRUE, NULL);
    dut_cb->dut_popup_nodelist->dut_p6_0size		
	= 0;
    dut_cb->dut_popup_nodelist->dut_p6_0num_entry	
	= 0;

    /* initialize popup 7 */
    dut_cb->dut_popup_dbattr
	= (DUT_P7_DBATTR *)MEreqmem(0, sizeof(DUT_P7_DBATTR), TRUE, NULL);

    /* initialize popup 8 */
    dut_cb->dut_popup_objattr				
	= (DUT_P8_OBJATTR *)MEreqmem(0, sizeof(DUT_P8_OBJATTR), TRUE, NULL);

    /* initialize popup 9 */
    dut_cb->dut_popup_viewattr				
	= (DUT_P9_VIEWATTR *)MEreqmem(0, sizeof(DUT_P9_VIEWATTR), TRUE, NULL);

    /* initialize popup 12 */
    dut_cb->dut_popup_ldblist			
	= (DUT_P12_LDBLIST *)MEreqmem(0, sizeof(DUT_P12_LDBLIST), TRUE, NULL);

    /* initialize popup 13 */
    dut_cb->dut_popup_ownlist			
	= (DUT_P13_OWNLIST *)MEreqmem(0, sizeof(DUT_P13_OWNLIST), TRUE, NULL);

    /* Browsing popups */
    /* initialize popup 14 */
    dut_cb->dut_popup_b_node			
	= (DUT_P14_NODE *)MEreqmem(0, sizeof(DUT_P14_NODE), TRUE, NULL);
    dut_cb->dut_popup_b_node->dut_p14_1node
	= (DUT_T14_NODE *)NULL;
    dut_cb->dut_popup_b_node->dut_p14_1node_curr_p
	= (DUT_T14_NODE *)MEreqmem(0, sizeof(DUT_T14_NODE), TRUE, NULL);
    dut_cb->dut_popup_b_node->dut_p14_0size
	= 0;

    /* initialize popup 15 */
    dut_cb->dut_popup_b_ldb			
	= (DUT_P15_LDB *)MEreqmem(0, sizeof(DUT_P15_LDB), TRUE, NULL);
    dut_cb->dut_popup_b_ldb->dut_p15_1ldb
	= (DUT_T15_LDB *)NULL;
    dut_cb->dut_popup_b_ldb->dut_p15_1ldb_curr_p
	= (DUT_T15_LDB *)MEreqmem(0, sizeof(DUT_T15_LDB), TRUE, NULL);
    dut_cb->dut_popup_b_ldb->dut_p15_0size
	= 0;

    /* initialize popup 16 */
    dut_cb->dut_popup_b_table			
	= (DUT_P16_TABLE *)MEreqmem(0, sizeof(DUT_P16_TABLE), TRUE, NULL);
    dut_cb->dut_popup_b_table->dut_p16_1table
	= (DUT_T16_TABLE *)NULL;
    dut_cb->dut_popup_b_table->dut_p16_1table_curr_p
	= (DUT_T16_TABLE *)MEreqmem(0, sizeof(DUT_T16_TABLE), TRUE, NULL);
    dut_cb->dut_popup_b_table->dut_p16_0size
	= 0;

    return(E_DU_OK);
}


/*
** Name: dut_ii5_init_helpfile	- Initialize the names of the help files used
**				in starview.
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      24-oct-1988 (alexc)
**          Initial Creation.
*/
DUT_STATUS
dut_ii5_init_helpfile(dut_cb, dut_errcb)
    DUT_CB	*dut_cb;
    DUT_ERRCB	*dut_errcb;
{
    char	dut_helppath[DDB_MAXPATH];
    char	dut_helppath2[DDB_MAXPATH];
    char	*dut_helpp_p;
    char	*dut_index;
    PTR		*alloc_p;

    if (dut_cb->dut_helpcb == NULL)
    {
	dut_cb->dut_helpcb =
		(DUT_HELPCB *)MEreqmem(0, sizeof(DUT_HELPCB), TRUE, NULL);
    }
    IIUGhlpname("svhf11.hlp", dut_cb->dut_helpcb->dut_hf1_1name);
    IIUGhlpname("svhf21.hlp", dut_cb->dut_helpcb->dut_hf2_1name);
    IIUGhlpname("svhf22.hlp", dut_cb->dut_helpcb->dut_hf2_2name);
    IIUGhlpname("svhf25.hlp", dut_cb->dut_helpcb->dut_hf2_5name);
    IIUGhlpname("svhf31.hlp", dut_cb->dut_helpcb->dut_hf3_1name);
    IIUGhlpname("svhp21.hlp", dut_cb->dut_helpcb->dut_hp2_1name);
    IIUGhlpname("svhp31.hlp", dut_cb->dut_helpcb->dut_hp3_1name);
    IIUGhlpname("svhp41.hlp", dut_cb->dut_helpcb->dut_hp4_1name);
    IIUGhlpname("svhp51.hlp", dut_cb->dut_helpcb->dut_hp5_1name);
    IIUGhlpname("svhp61.hlp", dut_cb->dut_helpcb->dut_hp6_1name);
    IIUGhlpname("svhp71.hlp", dut_cb->dut_helpcb->dut_hp7_1name);
    IIUGhlpname("svhp72.hlp", dut_cb->dut_helpcb->dut_hp7_2name);
    IIUGhlpname("svhp121.hlp", dut_cb->dut_helpcb->dut_hp12_1name);
    IIUGhlpname("svhp131.hlp", dut_cb->dut_helpcb->dut_hp13_1name);
    IIUGhlpname("svhp141.hlp", dut_cb->dut_helpcb->dut_hp14_1name);
    IIUGhlpname("svhp151.hlp", dut_cb->dut_helpcb->dut_hp15_1name);
    IIUGhlpname("svhp161.hlp", dut_cb->dut_helpcb->dut_hp16_1name);
    IIUGhlpname("svhp162.hlp", dut_cb->dut_helpcb->dut_hp16_2name);
    IIUGhlpname("svhu1.hlp", dut_cb->dut_helpcb->dut_hu1_1name);
    return(E_DU_OK);
}
