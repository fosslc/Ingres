/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <dbms.h>
#include    <er.h>
#include    <me.h>
#include    <st.h>
#include    <cm.h>
#include    <nm.h>
#include    <duerr.h>
EXEC SQL INCLUDE 'dut.sh';
EXEC SQL INCLUDE SQLCA;

/*
**
**  Name: DUTUTIL.SC - Utilities for DUT
**
**  Description:
**          dut_uu1_option - ** Removed **
**	    dut_uu2_dbop - Performs Database Connect/Disconnect, Direct
**			    connect/disconnect operations.
**	    dut_uu3_perror - ** Removed **
**	    dut_uu4_cleanup - close the form system and clean up.
**	    dut_uu5_chkdbname - check string for valid database name.
**	    dut_uu6_opmessage - ** Removed **
**	    dut_uu7_table_top - scroll to top of table field.
**	    dut_uu8_table_bottom - scroll to bottom of table field.
**	    dut_uu9_table_find - scroll to a specified row by match the field
**			     	to a string.
**	    dut_uu10_table_cdbname - ** Removed **
**	    dut_uu11_table_chk_criteria - checks a row against the 
**				selection criteria
**				and determine whether to load the row to the
**				table field.
**	    dut_uu12_empty_table - check to see if the table is empty.
**	    dut_uu13_buble_sort	- Sort an Array.
**	    dut_uu14_find_object - search objlist for a given object_name, and
**				owner, and sets a pointer pointing to it.
**	    dut_uu15_goto_curr_obj - Moves the cursor to the object specified
**				by function parameter.
**	    dut_uu16_get_vnode - Gets from the system, the II_INS_VNODE.
**	    dut_uu17_newldb - Adds the LDB information to LDB list if
**				the LDB is not already in the list.
**	    dut_uu18_chk_ldblist - Removes entry in the LDB list if there
**				is no object in the OBJ list that is from it.
**	    dut_uu19_test_ldb  - Test to see if a particular Local Database
**				is accessable(this also means that the node
**				of the LDB is accessible).
**	    dut_uu20_test_nodelist - Using dut_uu19_test_ldb() to test LDB
**				that have different nodename, alike ones
**				need only test one LDB.
**	    dut_uu21_test_node - test accessibility of a set of LDB in the same
**				node(single LDB test for whole set).
**	    dut_uu22_test_ldblist - test accessibility of a set of LDBs
**				individually.
**	    dut_uu23_reset_ldblist - reset the status field to "?".
**	    dut_uu24_gcf_node - get node names from GCF files.
**	    dut_uu25_insert_node - inserts a node into the node list.
**
**  History:
**      24-oct-1988 (alexc)
**          Initial Creation.
**	08-mar-1989 (alexc)
**	    Alpha test version.
**	22-mar-1989 (alexc)
**	    Added functions for TestNode option.
**	03-may-1989 (alexc)
**	    Added Browsing functions.
**	11-jul-1989 (alexc)
**	    Remove functions: dut_uu3_perror(), dut_uu6_opmessage(), and
**		dut_uu10_cdbname().  These functions are no longer used.
**	06-jun-1991 (fpang)
**	    Fixed dut_uu2_dbop() to always set prev_ldb, and prev_node
**	    when state changes to 'direct connected to anything'.
**	    Also, don't reset dut_on_error_exit because caller sets it.
**	    Fixes B37852.
**      08-30-93 (huffman)
**              add <me.h>
**      10-07-94 (newca01) release 1.1 fix bug 62778
**          Added code to init dut_u16_vnode in dut_uu16_get_vnode().
**      10-24-94  (newca01) release 1.1 fix bug 46986 
**          Added new routine dut_uu26_retry_dircont to fix textnode 
**          and testldb for gateway databases. 
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of nm.h plus return types to eliminate gcc 4.3i
**	    warnings.
*/

/*
** Name: dut_uu2_dbop	- determine operation on databases
**			  base on the flag.
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
**	06-jun-1991 (fpang)
**	    Fixed to always set prev_ldb, and prev_node
**	    when state changes to 'direct connected to anything'.
**	    Also, don't reset dut_on_error_exit because caller sets it.
*	    Fixes B37852.
*/
DUT_STATUS
dut_uu2_dbop(flag, dut_cb, dut_errcb)
DUT_FLAG    flag;
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{

    EXEC SQL begin declare section;
    char	dut_buffer[DDB_2MAXNAME];
    char	dut_buffer2[DDB_2MAXNAME];
    EXEC SQL end declare section;

    DUT_STATUS errcode;
    DUT_STATUS ret_status;

    ret_status = E_DU_OK;

    if (dut_cb->dut_c2_nodename[0] == EOS)
    {
	STprintf(dut_buffer, "%s/star", dut_cb->dut_c1_ddbname);
    }
    else if (!STcompare(dut_cb->dut_c2_nodename,
			dut_cb->dut_c13_vnodename))
    {
	STprintf(dut_buffer, "%s/star", dut_cb->dut_c1_ddbname);
    }
    else if (STcompare(dut_cb->dut_c2_nodename,
			dut_cb->dut_c13_vnodename))
    {
	STprintf(dut_buffer, "%s::%s/star",
			dut_cb->dut_c2_nodename,
			dut_cb->dut_c1_ddbname);
    }
    if (dut_cb->dut_c0_iidbdbnode[0] == EOS)
    {
	STcopy("iidbdb", dut_buffer2);
    }
    else
    {
	STprintf(dut_buffer2, "%s::%s", dut_cb->dut_c0_iidbdbnode, "iidbdb");
    }

    switch (flag)
    {
	case DUT_IIDBDBOPEN:
	    switch (dut_cb->dut_c7_status)
	    {
		case DUT_S_NULL:
			EXEC SQL connect :dut_buffer2;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;
			    break;
			}
			STcopy(dut_cb->dut_c0_iidbdbnode,
				dut_cb->dut_curr_iidbdbnode);
			dut_cb->dut_c7_status = DUT_S_CONNECT_IIDBDB;
			break;
		case DUT_S_CONNECT_DDB:
			EXEC SQL commit;
			EXEC SQL direct connect
			    with		
				node		= :dut_cb->dut_c0_iidbdbnode,
				database	= iidbdb;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			STcopy(dut_cb->dut_c0_iidbdbnode,
				dut_cb->dut_curr_iidbdbnode);
			STcopy(dut_cb->dut_c0_iidbdbnode,
				dut_cb->dut_c8_prev_ldbnode);
			STcopy("iidbdb",
			       dut_cb->dut_c9_prev_ldbname);
			dut_cb->dut_c7_status = DUT_S_DIRCONT_IIDBDB;
			if (sqlca.sqlcode == DUT_NONE_STAR_SERVER)
			{
			    dut_ee1_error(dut_errcb,
				E_DU5110_SV_NOT_STAR_SERVER, 0);
			}
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			break;
		case DUT_S_DIRCONT_LDB:
		case DUT_S_DIRCONT_CDB:
		case DUT_S_DIRCONT_IIDBDB:
			EXEC SQL whenever sqlerror call dut_ee5_none_starview;
			EXEC SQL commit;
			EXEC SQL direct disconnect;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			dut_cb->dut_c7_status = DUT_S_CONNECT_DDB;
			EXEC SQL commit;
			EXEC SQL direct connect with 
				node		= :dut_cb->dut_c0_iidbdbnode,
				database	= iidbdb;
			if (sqlca.sqlcode == DUT_NONE_STAR_SERVER)
			{
			    dut_ee1_error(dut_errcb, 
				E_DU5110_SV_NOT_STAR_SERVER, 0);
			}
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			STcopy(dut_cb->dut_c0_iidbdbnode,
				dut_cb->dut_curr_iidbdbnode);
			STcopy(dut_cb->dut_c0_iidbdbnode,
				dut_cb->dut_c8_prev_ldbnode);
			STcopy("iidbdb",
			       dut_cb->dut_c9_prev_ldbname);
			dut_cb->dut_c7_status = DUT_S_DIRCONT_IIDBDB;
			break;
		case DUT_S_CONNECT_IIDBDB:
			if (STcompare(dut_cb->dut_c0_iidbdbnode,
					dut_cb->dut_curr_iidbdbnode))
			{
			    EXEC SQL commit;
			    EXEC SQL disconnect;
			    if (sqlca.sqlcode < 0)
			    {
				ret_status = DUT_ERROR;			    
				break;
			    }
			    dut_cb->dut_c7_status = DUT_S_NULL;
			    EXEC SQL connect :dut_buffer2;
			    if (sqlca.sqlcode < 0)
			    {
				ret_status = DUT_ERROR;			    
				break;
			    }
			    STcopy(dut_cb->dut_c0_iidbdbnode,
					dut_cb->dut_curr_iidbdbnode);
			    dut_cb->dut_c7_status = DUT_S_CONNECT_IIDBDB;
			}
			break;
		default:
			break;
	    }
	    break;

	case DUT_IIDBDBCLOSE:
	    switch (dut_cb->dut_c7_status)
	    {
		case DUT_S_CONNECT_IIDBDB:
			EXEC SQL commit;
			EXEC SQL disconnect;
			dut_cb->dut_c7_status = DUT_S_NULL;
			break;
		case DUT_S_DIRCONT_IIDBDB:
			EXEC SQL commit;
			EXEC SQL direct disconnect;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			dut_cb->dut_c7_status = DUT_S_CONNECT_DDB;
			break;
		case DUT_S_NULL:
		case DUT_S_CONNECT_DDB:
		case DUT_S_DIRCONT_LDB:
		case DUT_S_DIRCONT_CDB:
			/* Error occur */
			ret_status = DUT_ERROR;			    
			break;
	    }
	    break;

	case DUT_DDBOPEN:
	    switch (dut_cb->dut_c7_status)
	    {
		case DUT_S_NULL:
			EXEC SQL connect :dut_buffer;
			STcopy(dut_cb->dut_c1_ddbname,
				dut_cb->dut_c11_prev_ddbname);
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			dut_cb->dut_c7_status = DUT_S_CONNECT_DDB;
			break;
		case DUT_S_CONNECT_IIDBDB:
			EXEC SQL disconnect;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			dut_cb->dut_c7_status = DUT_S_NULL;
			EXEC SQL connect :dut_buffer;
			STcopy(dut_cb->dut_c1_ddbname,
				dut_cb->dut_c11_prev_ddbname);
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			dut_cb->dut_c7_status = DUT_S_CONNECT_DDB;
			break;
		case DUT_S_CONNECT_DDB:
			if (STcompare(dut_cb->dut_c1_ddbname,
				dut_cb->dut_c11_prev_ddbname))
			{
			    EXEC SQL disconnect;
			    if (sqlca.sqlcode < 0)
			    {
				ret_status = DUT_ERROR;			    
				break;
			    }
			    EXEC SQL connect :dut_buffer;
			    STcopy(dut_cb->dut_c1_ddbname,
					dut_cb->dut_c11_prev_ddbname);
			    if (sqlca.sqlcode < 0)
			    {
				ret_status = DUT_ERROR;			    
				break;
			    }
			    dut_cb->dut_c7_status = DUT_S_CONNECT_DDB;
			}
			break;
		case DUT_S_DIRCONT_IIDBDB:
		case DUT_S_DIRCONT_LDB:
		case DUT_S_DIRCONT_CDB:
			EXEC SQL direct disconnect;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			dut_cb->dut_c7_status = DUT_S_CONNECT_DDB;
			if (STcompare(dut_cb->dut_c1_ddbname,
				dut_cb->dut_c11_prev_ddbname))
			{
			    EXEC SQL disconnect;
			    if (sqlca.sqlcode < 0)
			    {
				ret_status = DUT_ERROR;			    
				break;
			    }
			    dut_cb->dut_c7_status = DUT_S_NULL;
			    EXEC SQL connect :dut_buffer;
			    STcopy(dut_cb->dut_c1_ddbname,
				    dut_cb->dut_c11_prev_ddbname);
			    if (sqlca.sqlcode < 0)
			    {
				ret_status = DUT_ERROR;			    
				break;
			    }
			    EXEC SQL commit;
			    dut_cb->dut_c7_status = DUT_S_CONNECT_DDB;
			}
			break;
	    }
	    break;

	case DUT_DDBCLOSE:
	    switch (dut_cb->dut_c7_status)
	    {
		case DUT_S_DIRCONT_CDB:
		case DUT_S_DIRCONT_IIDBDB:
		case DUT_S_DIRCONT_LDB:
			EXEC SQL commit;
			EXEC SQL direct disconnect;
			dut_cb->dut_c7_status = DUT_S_CONNECT_DDB;
			EXEC SQL disconnect;
			dut_cb->dut_c7_status = DUT_S_NULL;
			break;
		case DUT_S_CONNECT_DDB:
			EXEC SQL commit;
			EXEC SQL disconnect;
			dut_cb->dut_c7_status = DUT_S_NULL;
			break;
		case DUT_S_CONNECT_IIDBDB:
			break;
		case DUT_S_NULL:
			break;
	    }
	    break;

	case DUT_DIRECT_CONNECT:
	    switch (dut_cb->dut_c7_status)
	    {
		case DUT_S_CONNECT_DDB:
			EXEC SQL commit;
			EXEC SQL direct connect
				with	
					node=:dut_cb->dut_c4_ldbnode,
					database=:dut_cb->dut_c3_ldbname;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			STcopy(dut_cb->dut_c4_ldbnode,
			       dut_cb->dut_c8_prev_ldbnode);
			STcopy(dut_cb->dut_c3_ldbname,
			       dut_cb->dut_c9_prev_ldbname);
			dut_cb->dut_c7_status = DUT_S_DIRCONT_LDB;
			break;
		case DUT_S_CONNECT_IIDBDB:
			break;
		case DUT_S_DIRCONT_IIDBDB:
		case DUT_S_DIRCONT_LDB:
		case DUT_S_DIRCONT_CDB:
			if (!STcompare(dut_cb->dut_c4_ldbnode, 
				dut_cb->dut_c8_prev_ldbnode) &&
			    !STcompare(dut_cb->dut_c3_ldbname, 
				dut_cb->dut_c9_prev_ldbname))
			{
			    ret_status = E_DU_OK;
			    break;
			    /* its already direct connect to LDB */
			}
			EXEC SQL commit;
			EXEC SQL direct disconnect;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			dut_cb->dut_c7_status = DUT_S_CONNECT_DDB;
			EXEC SQL commit;
			EXEC SQL direct connect
				with	
					node=:dut_cb->dut_c4_ldbnode,
					database=:dut_cb->dut_c3_ldbname;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			STcopy(dut_cb->dut_c4_ldbnode, 
				dut_cb->dut_c8_prev_ldbnode);
			STcopy(dut_cb->dut_c3_ldbname, 
				dut_cb->dut_c9_prev_ldbname);
			dut_cb->dut_c7_status = DUT_S_DIRCONT_LDB;
			break;
		case DUT_S_NULL:
			break;
	    }
	    break;

	case DUT_DIRECT_DISCONNECT:
	    switch (dut_cb->dut_c7_status)
	    {
		case DUT_S_DIRCONT_IIDBDB:
		case DUT_S_DIRCONT_LDB:
		case DUT_S_DIRCONT_CDB:
			EXEC SQL commit;
			EXEC SQL direct disconnect;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			dut_cb->dut_c4_ldbnode[0] = EOS;
			dut_cb->dut_c3_ldbname[0] = EOS;
			dut_cb->dut_c7_status = DUT_S_CONNECT_DDB;
			break;
		case DUT_S_NULL:
			break;
		case DUT_S_CONNECT_DDB:
			break;
		case DUT_S_CONNECT_IIDBDB:
			break;
	    }
	    break;

	case DUT_DIRECT_CONNECT_CDB:
	    switch (dut_cb->dut_c7_status)
	    {
		case DUT_S_CONNECT_DDB:
			EXEC SQL commit;
			EXEC SQL direct connect;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			STcopy(dut_cb->dut_c6_cdbnode, dut_cb->dut_c4_ldbnode);
			STcopy(dut_cb->dut_c5_cdbname, dut_cb->dut_c3_ldbname);
			STcopy(dut_cb->dut_c4_ldbnode, 
				dut_cb->dut_c8_prev_ldbnode);
			STcopy(dut_cb->dut_c3_ldbname, 
				dut_cb->dut_c9_prev_ldbname);
			dut_cb->dut_c7_status = DUT_S_DIRCONT_CDB;
			break;
		case DUT_S_DIRCONT_IIDBDB:
		case DUT_S_DIRCONT_LDB:
			EXEC SQL commit;
			EXEC SQL direct disconnect;
			dut_cb->dut_c7_status = DUT_S_NULL;
			EXEC SQL commit;
			EXEC SQL direct connect;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			STcopy(dut_cb->dut_c6_cdbnode, dut_cb->dut_c4_ldbnode);
			STcopy(dut_cb->dut_c5_cdbname, dut_cb->dut_c3_ldbname);
			STcopy(dut_cb->dut_c4_ldbnode, 
				dut_cb->dut_c8_prev_ldbnode);
			STcopy(dut_cb->dut_c3_ldbname, 
				dut_cb->dut_c9_prev_ldbname);
			dut_cb->dut_c7_status = DUT_S_DIRCONT_CDB;
			break;
		case DUT_S_DIRCONT_CDB:
			/* do nothing */
			break;
		case DUT_S_NULL:
			break;
	    }
	    break;

	case DUT_DISCONNECT:
	    switch (dut_cb->dut_c7_status)
	    {
		case DUT_S_CONNECT_IIDBDB:
		case DUT_S_CONNECT_DDB:
			EXEC SQL disconnect;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			dut_cb->dut_c7_status = DUT_S_NULL;
			break;
		case DUT_S_DIRCONT_IIDBDB:
		case DUT_S_DIRCONT_LDB:
		case DUT_S_DIRCONT_CDB:
			EXEC SQL direct disconnect;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			dut_cb->dut_c7_status = DUT_S_CONNECT_DDB;
			EXEC SQL disconnect;
			if (sqlca.sqlcode < 0)
			{
			    ret_status = DUT_ERROR;			    
			    break;
			}
			dut_cb->dut_c7_status = DUT_S_NULL;
			break;
		case DUT_S_NULL:
			break;
	    }
	    break;
	default:
	    break;
    }
    if ((dut_cb->dut_c15_username[0] == EOS) &&
	(dut_cb->dut_c7_status != DUT_S_NULL))
    {
	EXEC SQL select dbmsinfo('username') into :dut_cb->dut_c15_username;
    }
    dut_cb->dut_on_error_exit = FALSE;
    return(ret_status);
}


/*
** Name: dut_uu4_cleanup	- determine operation on databases
**			  base on the flag.
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
dut_uu4_cleanup(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    dut_uu2_dbop(DUT_DDBCLOSE, dut_cb, dut_errcb);

    if (dut_cb->dut_form_intro->dut_f1_4ddblist != NULL)
	MEfree(dut_cb->dut_form_intro->dut_f1_4ddblist);
    if (dut_cb->dut_form_intro != NULL)
	MEfree(dut_cb->dut_form_intro);
    if (dut_cb->dut_form_ldblist->dut_f2_3ldblist != NULL)
	MEfree(dut_cb->dut_form_ldblist->dut_f2_3ldblist);
    if (dut_cb->dut_form_ldblist != NULL)
        MEfree(dut_cb->dut_form_ldblist);
    if (dut_cb->dut_form_objlist->dut_f3_9objlist != NULL)
        MEfree(dut_cb->dut_form_objlist->dut_f3_9objlist);
    if (dut_cb->dut_form_objlist != NULL)
        MEfree(dut_cb->dut_form_objlist);
    if (dut_cb->dut_popup_select != NULL)
	MEfree(dut_cb->dut_popup_select);
    if (dut_cb->dut_popup_nodelist != NULL)
    {
	if (dut_cb->dut_popup_nodelist->dut_p6_1nodelist != NULL)
	    MEfree(dut_cb->dut_popup_nodelist->dut_p6_1nodelist);
	if (dut_cb->dut_popup_nodelist->dut_p6_1nodelist_curr_p != NULL)
	    MEfree(dut_cb->dut_popup_nodelist->dut_p6_1nodelist_curr_p);
	MEfree(dut_cb->dut_popup_nodelist);
    }
    if (dut_cb->dut_popup_dbattr != NULL)
	MEfree(dut_cb->dut_popup_dbattr);
    if (dut_cb->dut_popup_objattr != NULL)
	MEfree(dut_cb->dut_popup_objattr);
    if (dut_cb->dut_popup_viewattr != NULL)
	MEfree(dut_cb->dut_popup_viewattr);

    EXEC FRS clear screen;
    EXEC FRS endforms;
    return(E_DU_OK);
}


/*
** Name: dut_uu5_chkdbname	- determine operation on databases
**			  base on the flag.
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
dut_uu5_chkdbname(dbname, dut_errcb)
char	*dbname;
DUT_ERRCB   *dut_errcb;
{
    register	char	*p;

    p = dbname;
    if ((STlength(p) > DB_MAXNAME) || (*p == EOS))
	return(DUT_ERROR);

    if (!CMalpha(p))
	return(DUT_ERROR);

    for (CMnext(p); *p != EOS; CMnext(p))
    {
    	if (!CMnmchar(p))
	    return (DUT_ERROR);
    }

    return(E_DU_OK);
}


/*
** Name: dut_uu7_table_top	- Scroll Table to top of Table field.
**
** Description:
**		If the parameter dut_u7_1field_name is NULL, this function will
**	check if the current field in the current form is a table field
**	and then goto the top of the table field.  Otherwise it will check the
**	field provide in dut_u7_1field_name and scroll to the top of that
**	table field.
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
**      5-jan-1989 (alexc)
**          Initial Creation.
*/
DUT_STATUS
dut_uu7_table_top(dut_u7_1field_name)
EXEC SQL begin declare section;
char	*dut_u7_1field_name;
EXEC SQL end declare section;
{
    EXEC SQL begin declare section;
    char	dut_u7_1curform[DB_MAXNAME];
    char	dut_u7_2curfield[DB_MAXNAME];
    i4		dut_u7_3tablefield;
    EXEC SQL end declare section;


    if (dut_u7_1field_name != NULL)
    {
	STcopy(dut_u7_1field_name, dut_u7_2curfield);
	EXEC FRS inquire_frs form
	    (
	    :dut_u7_1curform = name
	    );
    }
    else
    {
	EXEC FRS inquire_frs form
	    (
	    :dut_u7_1curform = name,
	    :dut_u7_2curfield = field
	    );
    }

    if (dut_u7_1curform[0] == EOS || dut_u7_2curfield[0] == EOS)
    {
	return(DUT_ERROR);
    }

    EXEC FRS inquire_frs field :dut_u7_1curform
    (
	:dut_u7_3tablefield = table(:dut_u7_2curfield)
    );

    if (dut_u7_3tablefield == 0)
    {
	/* Field is not a table field */
	return(DUT_ERROR);
    }
    else
    {
	EXEC FRS scroll :dut_u7_1curform :dut_u7_2curfield to 1;
    }
}


/*
** Name: dut_uu8_table_bottom	- Scroll to the bottom of a Table field.
**
** Description:
**		If the parameter dut_u8_1field_name is NULL, this function will
**	check if the current field in the current form is a table field
**	and then goto the bottom of the table field.  Otherwise it will check the
**	field provide in dut_u8_1field_name and scroll to the bottom of that
**	table field.
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
**      5-jan-1989 (alexc)
**          Initial Creation.
*/
DUT_STATUS
dut_uu8_table_bottom(dut_u8_1field_name)
EXEC SQL begin declare section;
char	*dut_u8_1field_name;
EXEC SQL end declare section;
{
    EXEC SQL begin declare section;
    char	dut_u8_1curform[DB_MAXNAME];
    char	dut_u8_2curfield[DB_MAXNAME];
    i4		dut_u8_3tablefield;
    EXEC SQL end declare section;

    if (dut_u8_1field_name != NULL)
    {
	STcopy(dut_u8_1field_name, dut_u8_2curfield);
	EXEC FRS inquire_frs form
	    (
	    :dut_u8_1curform = name
	    );
    }
    else
    {
	EXEC FRS inquire_frs form
	    (
	    :dut_u8_1curform = name,
	    :dut_u8_2curfield = field
	    );
    }

    if (dut_u8_1curform[0] == EOS || dut_u8_2curfield[0] == EOS)
    {
	return(DUT_ERROR);
    }

    EXEC FRS inquire_frs field :dut_u8_1curform
    (
	:dut_u8_3tablefield = table(:dut_u8_2curfield)
    );

    if (dut_u8_3tablefield == 0)
    {
	/* Field is not a table field */
	return(DUT_ERROR);
    }
    else
    {
	EXEC FRS scroll :dut_u8_1curform :dut_u8_2curfield to end;
    }
}


/*
** Name: dut_uu9_table_find	- Search for a given string in a
**					column of the table field.
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
**      5-jan-1989 (alexc)
**          Initial Creation.
*/
DUT_STATUS
dut_uu9_table_find(dut_cb, dut_errcb,
	dut_u9_0field, dut_u9_1field_name, dut_u9_2match_string)
EXEC SQL begin declare section;
DUT_CB	*dut_cb;
DUT_ERRCB *dut_errcb;
char	*dut_u9_0field;
char	*dut_u9_1field_name;
char	*dut_u9_2match_string;
EXEC SQL end declare section;
{
    EXEC SQL begin declare section;
    char	dut_u9_0string[DB_MAXNAME + 4];
    char	dut_u9_0prompt[DB_MAXNAME + 4];
    char	dut_u9_1curform[DB_MAXNAME + 4];
    char	dut_u9_2curfield[DB_MAXNAME + 4];
    i4		dut_u9_3tablefield;
    i4		dut_u9_4cur_row;
    char	dut_u9_6curcolumn[DB_MAXNAME + 4];
    char	dut_u9_7curvalue[DB_MAXNAME + 4];
    i4		dut_u9_8col_num;
    i4		dut_u9_9foundrow;
    i4		dut_u9_10datatype;
    EXEC SQL end declare section;
    i4		dut_u9_5found;

    dut_u9_5found = FALSE;

    if (dut_u9_1field_name != NULL)
    {
	EXEC FRS inquire_frs form
	    (
	    :dut_u9_1curform = name
	    );
	STcopy(dut_u9_1field_name, dut_u9_2curfield);
    }
    else
    {
	EXEC FRS inquire_frs form
	    (
	    :dut_u9_1curform = name,
	    :dut_u9_2curfield = field
	    );
    }

    if (dut_u9_1curform[0] == EOS || dut_u9_2curfield[0] == EOS)
    {
	return(DUT_ERROR);
    }

    EXEC FRS inquire_frs field :dut_u9_1curform
    (
	:dut_u9_3tablefield = table(:dut_u9_2curfield)
    );

    if (dut_u9_3tablefield == 0)
    {
	/* Field is not a table field */
	return(DUT_ERROR);
    }
    else
    {
	EXEC FRS inquire_frs table :dut_u9_1curform
	    (
	    :dut_u9_4cur_row = rowno(:dut_u9_2curfield)
	    );

	EXEC FRS inquire_frs column :dut_u9_1curform :dut_u9_2curfield
	    (
	    :dut_u9_6curcolumn	= name,
	    :dut_u9_10datatype	= datatype,
	    :dut_u9_8col_num	= number
	    );

	if (!(dut_u9_10datatype == DUT_32_C ||
		dut_u9_10datatype == DUT_37_VCHAR ||
		dut_u9_10datatype == DUT_20_CHAR ||
		dut_u9_10datatype == DUT_21_VARCHAR ||
		dut_u9_10datatype == DUT_3_DATE ||
		dut_u9_10datatype == DUT_NULL_32_C ||
		dut_u9_10datatype == DUT_NULL_37_VCHAR ||
		dut_u9_10datatype == DUT_NULL_20_CHAR ||
		dut_u9_10datatype == DUT_NULL_21_VARCHAR ||
		dut_u9_10datatype == DUT_NULL_3_DATE))
	{
	    return(DUT_ERROR);
	}

	if (dut_u9_2match_string == NULL)
	{
	    STprintf(dut_u9_0prompt, DUT_M41_PROMPT00, dut_u9_0field);
	    EXEC FRS prompt
		(
		:dut_u9_0prompt,
		:dut_u9_0string
		);
	}
	else
	{
	    STcopy(dut_u9_2match_string, dut_u9_0string);
	}

	if (dut_u9_0string[0] != EOS)
	{
	    EXEC FRS unloadtable :dut_u9_1curform :dut_u9_2curfield
		(
		:dut_u9_7curvalue = :dut_u9_6curcolumn,
		:dut_u9_9foundrow = _RECORD
		);
	    EXEC FRS begin;
		if (!STcompare(dut_u9_7curvalue, dut_u9_0string))
		{
		    EXEC FRS scroll :dut_u9_1curform :dut_u9_2curfield
			to :dut_u9_9foundrow;
		    dut_u9_4cur_row = dut_u9_9foundrow;
		    dut_u9_5found = TRUE;
		}
	    EXEC FRS end;

	    if (dut_u9_5found == FALSE)
	    {
	        EXEC FRS scroll :dut_u9_1curform :dut_u9_2curfield
		    to :dut_u9_4cur_row;
        	dut_ee1_error(dut_errcb, W_DU181C_SV_CANT_FIND_STRING,0);
	    }
	}
    }
}


/*
** Name: dut_uu11_chk_criteria	- Search for a given string in a
**					column of the table field.
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
**      5-jan-1989 (alexc)
**          Initial Creation.
*/
DUT_STATUS
dut_uu11_chk_criteria(dut_cb, dut_errcb, dut_f3_0objlist_curr_p)
EXEC SQL begin declare section;
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
DUT_T3_OBJLIST	*dut_f3_0objlist_curr_p;
EXEC SQL end declare section;
{
    EXEC SQL begin declare section;
    DUT_F3_OBJLIST	*dut_f3_1objlist_p;
    EXEC SQL end declare section;
    i4  dut_f3_0select=FALSE;
    i4	dut_objtype=DUT_UNKNOWN_OBJ;
    i4  dut_show_system=FALSE;
    i4  dut_show_table=FALSE;
    i4  dut_show_view=FALSE;
    i4  dut_show_index=TRUE;  /* Not use yet */
    i4  dut_show_replic=TRUE; /* Not use yet */
    i4  dut_show_part=TRUE;   /* Not use yet */
    i4  dut_show_native=FALSE; /* Not use anymore */
    i4  dut_show_regist=FALSE; /* Not use anymore */

    dut_f3_1objlist_p = dut_cb->dut_form_objlist;

    dut_show_system= (!STcompare(dut_f3_1objlist_p->dut_f3_7show_system, "y"));
    dut_show_table = (!STcompare(dut_f3_1objlist_p->dut_f3_6show_table, "y"));
    dut_show_view  = (!STcompare(dut_f3_1objlist_p->dut_f3_5show_view, "y"));

    switch (dut_f3_0objlist_curr_p->dut_t3_0subtype[0])
    {
	case 'L':	/* Registered object */
	case 'l':
		switch (dut_f3_0objlist_curr_p->dut_t3_0objtype[0])
		{
			case 'V':	/* registered View */
			case 'v':	/* registered View */
				dut_objtype = DUT_REG_VIEW;
				break;
			case 'T':	/* registered Table */
			case 't':	/* registered Table */
				dut_objtype = DUT_REG_TABLE;
				break;
			case 'I':	/* Index object */
			case 'i':	/* Index object */
				dut_objtype = DUT_INDEX;
				break;
			case 'P':	/* Partitioned table */
				dut_objtype = DUT_PART_TABLE;
				break;
			case 'R':	/* Replicated table */
				dut_objtype = DUT_REPLIC_TABLE;
				break;
			default:
				dut_objtype = DUT_UNKNOWN_OBJ;
				break;
		}
		break;
	case 'N':	/* Native object */
	case 'n':
		switch (dut_f3_0objlist_curr_p->dut_t3_0objtype[0])
		{
			case 'V':	/* DD View object */
			case 'v':	/* DD View object */
				dut_objtype = DUT_DD_VIEW;
				break;
			case 'T':	/* DD Table object */
			case 't':	/* DD Table object */
				dut_objtype = DUT_DD_TABLE;
				break;
			case 'I':	/* Index object */
			case 'i':	/* Index object */
				dut_objtype = DUT_INDEX;
				break;
			case 'P':	/* Partitioned table */
				dut_objtype = DUT_PART_TABLE;
				break;
			case 'R':	/* Replicated table */
				dut_objtype = DUT_REPLIC_TABLE;
				break;
			default:
				dut_objtype = DUT_UNKNOWN_OBJ;
				break;
		}
		break;
	default:
		dut_objtype = DUT_UNKNOWN_OBJ;
		break;

    }
    switch (dut_objtype)
    {
	case DUT_DD_VIEW:
	case DUT_REG_VIEW:
		dut_f3_0select = dut_show_view;
		STcopy(DUT_M24_VIEW,
			dut_f3_0objlist_curr_p->dut_t3_2objtype);
		break;
	case DUT_DD_TABLE:
	case DUT_REG_TABLE:
		dut_f3_0select = dut_show_table;
		STcopy(DUT_M22_TABLE,
			dut_f3_0objlist_curr_p->dut_t3_2objtype);
		break;
	case DUT_INDEX:
		dut_f3_0select = dut_show_index;
		STcopy(DUT_M26_INDEX,
			dut_f3_0objlist_curr_p->dut_t3_2objtype);
		break;
	case DUT_PART_TABLE:
		dut_f3_0select = dut_show_part;
		STcopy(DUT_M42_PARTITION,
			dut_f3_0objlist_curr_p->dut_t3_2objtype);
		break;
	case DUT_REPLIC_TABLE:
		dut_f3_0select = dut_show_replic;
		STcopy(DUT_M43_REPLICATE,
			dut_f3_0objlist_curr_p->dut_t3_2objtype);
		break;
	case DUT_UNKNOWN_OBJ:
		STcopy(DUT_M20_UNKNOWN,
			dut_f3_0objlist_curr_p->dut_t3_2objtype);
		break;
    }

    switch (dut_f3_0objlist_curr_p->dut_t3_0system[0])
    {
	case 'Y':
	case 'y':
		dut_f3_0select = dut_show_system;
		STcopy(DUT_M38_SYSTEM,
			dut_f3_0objlist_curr_p->dut_t3_2objtype);
		break;
	default:
		/* Do nothing */
		break;
    }
    /* This section sets select according to the nodename, and database name
    ** of the selection criteria.
    */
    if (STcompare(dut_f3_1objlist_p->dut_f3_1nodename, "*") &&
		dut_f3_0objlist_curr_p->dut_t3_0ldbnode[0] != EOS)
    {
	if (STcompare(dut_f3_1objlist_p->dut_f3_1nodename,
		dut_f3_0objlist_curr_p->dut_t3_0ldbnode))
	{
	    dut_f3_0select = FALSE;
	}
    }
    if (STcompare(dut_f3_1objlist_p->dut_f3_2dbname, "*") &&
		dut_f3_0objlist_curr_p->dut_t3_0ldbname[0] != EOS)
    {
	if (STcompare(dut_f3_1objlist_p->dut_f3_2dbname,
		dut_f3_0objlist_curr_p->dut_t3_0ldbname))
	{
	    dut_f3_0select = FALSE;
	}
    }

    /* Select by owner of object */
    if (STcompare(dut_f3_1objlist_p->dut_f3_3owner, "*"))
    {
	if (STcompare(dut_f3_0objlist_curr_p->dut_t3_0objowner,
		dut_f3_1objlist_p->dut_f3_3owner))
	{
	    dut_f3_0select = FALSE;
	}
    }
    dut_f3_0objlist_curr_p->dut_t3_0select = dut_f3_0select;
}


/*
** Name: dut_uu12_empty_table	- Search for a given string in a
**					column of the table field.
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
**      5-jan-1989 (alexc)
**          Initial Creation.
*/
DUT_STATUS
dut_uu12_empty_table(dut_cb, dut_errcb, dut_u12_1field_name)
EXEC SQL begin declare section;
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
char		*dut_u12_1field_name;
EXEC SQL end declare section;
{
    EXEC SQL begin declare section;
    char	dut_u12_1curform[DB_MAXNAME];
    char	dut_u12_2curfield[DB_MAXNAME];
    i4		dut_u12_3tablefield;
    i4		dut_u12_4nrow;
    EXEC SQL end declare section;

    dut_u12_4nrow = 0;

    if (dut_u12_1field_name != NULL)
    {
	STcopy(dut_u12_1field_name, dut_u12_2curfield);
	EXEC FRS inquire_frs form
	    (
	    :dut_u12_1curform = name
	    );
    }
    else
    {
	EXEC FRS inquire_frs form
	    (
	    :dut_u12_1curform = name,
	    :dut_u12_2curfield = field
	    );
    }

    if (dut_u12_1curform[0] == EOS || dut_u12_2curfield[0] == EOS)
    {
	return(DUT_ERROR);
    }

    EXEC FRS inquire_frs field :dut_u12_1curform
    (
	:dut_u12_3tablefield = table(:dut_u12_2curfield)
    );

    if (dut_u12_3tablefield == DUT_0_NONE_TABLE_TYPE)
    {
	/* Field is not a table field */
	return(DUT_ERROR);
    }
    else
    {
	EXEC FRS inquire_frs table :dut_u12_1curform
	(
	:dut_u12_4nrow = lastrow(:dut_u12_2curfield)
	);
	return(dut_u12_4nrow);
    }
    return(dut_u12_4nrow);
}


/*
** Name: dut_uu13_buble_sort	- Sort object list.
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
**      5-jan-1989 (alexc)
**          Initial Creation.
*/
DUT_STATUS
dut_uu13_buble_sort(block_ptr, block_size, len_index, n_item)
char	*block_ptr;
int	block_size;
int	len_index;
int	n_item;
{
    int i = 0, j = 0;
    char *bki, *bkj;
    char *swap_blk;

    swap_blk = (char *)MEreqmem(0, block_size, TRUE, NULL);

    for (i = 0; i < (n_item - 1); i++)
    {
	for (j = 0; ((j+1) < (n_item-i)); j++)
	{
	    bki = block_ptr + (j * block_size);
	    bkj = block_ptr + ((j+1) * block_size);
	    if (STscompare(bki, len_index, bkj, len_index) > 0)
	    {
		MEcopy(bki, block_size, swap_blk);
		MEcopy(bkj, block_size, bki);
		MEcopy(swap_blk, block_size, bkj);
	    }
	}
    }
    MEfree(swap_blk);
    return(E_DU_OK);
}


/*
** Name: dut_uu14_find_object	- 
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
**      5-jan-1989 (alexc)
**          Initial Creation.
*/
DUT_STATUS
dut_uu14_find_object(dut_cb, dut_errcb, dut_t3_found_p, dut_owner, dut_obj)
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
DUT_T3_OBJLIST	**dut_t3_found_p;
char		*dut_owner;
char		*dut_obj;
{
    DUT_T3_OBJLIST	*dut_t3_p;
    DUT_F3_OBJLIST	*dut_f3_p;
    i4			dut_t3_i;

    dut_f3_p = dut_cb->dut_form_objlist;
    dut_t3_p = dut_f3_p->dut_f3_9objlist;
    *dut_t3_found_p = NULL;

    for (dut_t3_i = 0;
	dut_t3_i < dut_f3_p->dut_f3_0obj_num_entry;
	dut_t3_i++, dut_t3_p++)
    {
	if (!STcompare(dut_t3_p->dut_t3_0objowner, dut_owner) &&
		!STcompare(dut_t3_p->dut_t3_1objname, dut_obj))
	{
	    *dut_t3_found_p = dut_t3_p;
	    return(E_DU_OK);
	}	    
    }
    return(E_DU_OK);
}


/*
** Name: dut_uu15_goto_curr_obj	- goto row with specify object.
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
**      5-jan-1989 (alexc)
**          Initial Creation.
*/
DUT_STATUS
dut_uu15_goto_curr_obj(dut_cb, dut_errcb, dut_owner, dut_obj)
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
char		*dut_owner;
char		*dut_obj;
{
    EXEC SQL begin declare section;
    i4			dut_u15_row;
    i4			dut_u15_found;
    i4			dut_u15_curr_row;
    char		dut_u15_objname[DDB_MAXNAME];
    char		dut_u15_objowner[DDB_MAXNAME];
    EXEC SQL end declare section;

    EXEC FRS inquire_frs table dut_f3_objlist
	(
	:dut_u15_curr_row = rowno(dut_f3_9objlist)
	);

    EXEC FRS unloadtable dut_f3_objlist dut_f3_9objlist
	(
	:dut_u15_objname = dut_t3_1objname,
	:dut_u15_objowner= dut_t3_0objowner,
	:dut_u15_row	 = _RECORD
	);
    EXEC FRS begin;
	if (!STcompare(dut_u15_objowner, dut_owner) &&
		!STcompare(dut_u15_objname, dut_obj))
	{
	    EXEC FRS scroll dut_f3_objlist dut_f3_9objlist
		to :dut_u15_row;
	    dut_u15_found = TRUE;
	}
    EXEC FRS end;

    if (dut_u15_found == FALSE)
    {
	EXEC FRS scroll dut_f3_objlist dut_f3_9objlist
	    to :dut_u15_curr_row;
    }

    return(E_DU_OK);
}


/*
** Name: dut_uu16_get_vnode	- goto row with specify object.
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
**      5-jan-1989 (alexc)
**          Initial Creation.
**      10-07-94 (newca01) Release 1.1 fix bug 62778
**          Added code to init dut_u16_vnode.
**	17-Jun-2004 (schka24)
**	    Safer env var handling.
*/
DUT_STATUS
dut_uu16_get_vnode(dut_cb, dut_errcb)
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
{
    EXEC SQL begin declare section;
    char	dut_u16_vnode[DDB_MAXNAME];
    char	dut_u16_v_log_node[DDB_MAXNAME];
    char	*dut_u16_vnode_p;
    char	*dut_installation;
    EXEC SQL end declare section;

    dut_u16_vnode[0] = EOS;     /* 10-07-94 (newca01) B62778  */

    STcopy(DUT_M44_VNODE00, dut_u16_v_log_node);
    if (dut_cb->dut_c13_vnodename[0] == EOS)
        NMgtAt(dut_u16_v_log_node, &dut_u16_vnode_p);
    else
	dut_u16_vnode_p = dut_cb->dut_c13_vnodename;

    if (dut_u16_vnode_p == NULL || *dut_u16_vnode_p == EOS)
    {
	NMgtAt(DUT_M45_II_INSTALL, &dut_installation);
	STlpolycat(3, sizeof(dut_u16_v_log_node)-1, DUT_M46_II_GCN,
		(dut_installation == NULL ? "" : dut_installation),
		DUT_M47_LCL_VNODE,
		dut_u16_v_log_node);
	if (dut_cb->dut_c13_vnodename[0] == EOS)
	    NMgtAt(dut_u16_v_log_node, &dut_u16_vnode_p);
    }

    if (dut_u16_vnode_p == NULL || *dut_u16_vnode_p == EOS)
    {
	dut_ee1_error(dut_errcb, W_DU180D_SV_CANT_FIND_VNODE, 0);
	while (dut_u16_vnode[0] == EOS)
	{
	    /* FIXME overrun possible here, checked for by check-str but
	    ** perhaps we should just exit on toolong string.
	    */
	    EXEC FRS prompt ('Please enter VNODE name:',
				:dut_u16_vnode);
	    if (dut_ii2_check_str(DUT_CK_NODENAME, dut_u16_vnode, dut_errcb)
		== W_DU1802_SV_BAD_NODENAME)
	    {
		dut_ee1_error(dut_errcb, W_DU180E_SV_ENTER_BAD_VNODE, 0);
		dut_u16_vnode[0] = EOS;
	    }
	}
    	STlcopy(dut_u16_vnode, dut_cb->dut_c13_vnodename, sizeof(dut_cb->dut_c13_vnodename)-1);
	/* fix_me:  Define II_INS_VNODE logical or system symbol */
    }
    else
    {
	STlcopy(dut_u16_vnode_p, dut_cb->dut_c13_vnodename, sizeof(dut_cb->dut_c13_vnodename)-1);
    }

    return(E_DU_OK);
}


/*
** Name: dut_uu17_newldb	- adds the LDB name and LDB node to the list
**				  of LDBs pointed by dut_cb->dut_popup_ldblist.
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
**      5-jan-1989 (alexc)
**          Initial Creation.
*/
DUT_STATUS
dut_uu17_newldb(dut_cb, dut_errcb)
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
{
    EXEC SQL begin declare section;
    DUT_T2_LDBLIST	*dut_t2_append_p;
    DUT_P3_REGISTER	*dut_p3_p;
    EXEC SQL end declare section;
    DUT_F2_LDBLIST	*dut_f2_p;
    DUT_T2_LDBLIST	*dut_t2_p;
    DUT_T2_LDBLIST	*dut_t2_new_p;
    i4			dut_t2_cnt;
    i4			dut_t2_sz;

    dut_f2_p = dut_cb->dut_form_ldblist;
    dut_t2_p = dut_f2_p->dut_f2_3ldblist;
    dut_p3_p = dut_cb->dut_popup_register;
    dut_t2_cnt = dut_f2_p->dut_f2_0ldb_num_entry;
    dut_t2_sz  = dut_f2_p->dut_f2_0ldb_size;

    STtrmwhite(dut_t2_p->dut_t2_2nodename);
    STtrmwhite(dut_t2_p->dut_t2_3ldbname);
    STtrmwhite(dut_p3_p->dut_p3_4node);
    STtrmwhite(dut_p3_p->dut_p3_5database);

    if ((dut_t2_p->dut_t2_2nodename[0] == EOS) ||
	(dut_t2_p->dut_t2_3ldbname[0] == EOS))
    {
	return;
    }
    while (dut_t2_cnt--)
    {
	if (!STcompare(dut_t2_p->dut_t2_2nodename, dut_p3_p->dut_p3_4node) &&
	    !STcompare(dut_t2_p->dut_t2_3ldbname, dut_p3_p->dut_p3_5database))
	{
	    /* LDB already in LDB list */
	    return(E_DU_OK);
	}
	dut_t2_p++;
    }
    /* Case when the new LDB is the last LDB in the LDBlist */
    if (!STcompare(dut_t2_p->dut_t2_2nodename, dut_p3_p->dut_p3_4node) &&
	!STcompare(dut_t2_p->dut_t2_3ldbname, dut_p3_p->dut_p3_5database))
    {
	/* LDB already in LDB list */
	return(E_DU_OK);
    }
    /* LDB not in LDB list, do insert new info into LDB list */

    dut_t2_cnt = dut_f2_p->dut_f2_0ldb_num_entry;
    dut_t2_p = dut_f2_p->dut_f2_3ldblist;

    if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU1805_SV_CANT_OPEN_DDB, 2, 0, 
                      dut_cb->dut_c1_ddbname);
    }
    if (dut_t2_p == NULL || ((dut_t2_cnt+1) > dut_t2_sz))
    {
	dut_t2_new_p = (DUT_T2_LDBLIST *)MEreqmem(0,
		(dut_t2_cnt + DUT_ME_EXTRA) * sizeof(DUT_T2_LDBLIST),
		TRUE, NULL);
	if (dut_t2_p != NULL)
	{
	    MEcopy(dut_t2_p,
		sizeof(DUT_T2_LDBLIST)*dut_t2_sz,
		dut_t2_new_p);
	    /* Deallocate, and reallocate */
	    MEfree(dut_t2_p);
	    dut_t2_p = NULL;
	}
	dut_t2_p = dut_t2_new_p;
	dut_f2_p->dut_f2_3ldblist = dut_t2_p;
	dut_cb->dut_form_ldblist->dut_f2_0ldb_size 
		= dut_t2_cnt + DUT_ME_EXTRA;
    }

    dut_f2_p->dut_f2_0modify = TRUE;

    dut_t2_append_p = (DUT_T2_LDBLIST *)(dut_t2_p + dut_t2_cnt);

    EXEC SQL 
	SELECT distinct
	    ldb_id,
	    trim(ldb_node),
	    trim(ldb_database),
	    trim(ldb_dbms)
	into
	    :dut_t2_append_p->dut_t2_0ldbid,
	    :dut_t2_append_p->dut_t2_2nodename,
	    :dut_t2_append_p->dut_t2_3ldbname,
	    :dut_t2_append_p->dut_t2_4ldbtype
	from 
	    iiddb_ldbids
	where
	    ldb_node = :dut_p3_p->dut_p3_4node
	        and
	    ldb_database = :dut_p3_p->dut_p3_5database;

    STcopy(DUT_M50_UP, dut_t2_append_p->dut_t2_1status);

    dut_cb->dut_form_ldblist->dut_f2_0ldb_num_entry++;

    dut_uu13_buble_sort(dut_cb->dut_form_ldblist->dut_f2_3ldblist,
		sizeof(DUT_T2_LDBLIST), DDB_MAXNAME, 
		dut_cb->dut_form_ldblist->dut_f2_0ldb_num_entry);

    return(E_DU_OK);
}


/*
** Name: dut_uu18_chk_ldblist	- This function removes LDB from LDBlist if
**				  needed.
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
**      3-mar-1989 (alexc)
**          Initial Creation.
*/
DUT_STATUS
dut_uu18_chk_ldblist(dut_cb, dut_errcb)
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
{
    EXEC SQL begin declare section;
    DUT_F2_LDBLIST	*dut_f2_p;
    DUT_T2_LDBLIST	*dut_t2_p;
    DUT_T2_LDBLIST	*dut_t2_last_p;
    DUT_F3_OBJLIST	*dut_f3_p;
    DUT_T3_OBJLIST	*dut_t3_p;
    DUT_T3_OBJLIST	*dut_t3_curr_p;
    i4			dut_t2_cnt;
    i4			dut_t3_cnt;
    EXEC SQL end declare section;

    dut_f2_p = dut_cb->dut_form_ldblist;
    dut_t2_p = dut_f2_p->dut_f2_3ldblist;
    dut_f3_p = dut_cb->dut_form_objlist;
    dut_t3_p = dut_f3_p->dut_f3_9objlist;
    dut_t3_curr_p = dut_f3_p->dut_f3_9objlist_curr_p;
    dut_t2_cnt = dut_f2_p->dut_f2_0ldb_num_entry;
    dut_t3_cnt = dut_f3_p->dut_f3_0obj_num_entry;
    dut_t2_last_p = (DUT_T2_LDBLIST *)(dut_t2_p + dut_t2_cnt - 1);

    /* We wanted to remove the LDBlist entry only if there is no more
    ** objects in OBJLIST has references to it.
    */
    while (dut_t3_cnt--)
    {
	if (!STcompare(dut_t3_p->dut_t3_0ldbname,
			dut_t3_curr_p->dut_t3_0ldbname) &&
	    !STcompare(dut_t3_p->dut_t3_0ldbnode,
			dut_t3_curr_p->dut_t3_0ldbnode) &&
	    (dut_t3_p->dut_t3_0delete == FALSE))
	{
	    /* ldb still has object registered in the database */
	    return(E_DU_OK);
	}
	dut_t3_p++;
    }

    /* Find the LDB pointer */
    while (dut_t2_cnt--)
    {
	if (!STcompare(dut_t2_p->dut_t2_2nodename,
			dut_t3_curr_p->dut_t3_0ldbnode) &&
	    !STcompare(dut_t2_p->dut_t2_3ldbname,
			dut_t3_curr_p->dut_t3_0ldbname))
	{
	    /* found the ldb pointer */
	    break;
	}
	(DUT_T2_LDBLIST *)dut_t2_p++;
    }

    dut_f2_p->dut_f2_0modify = TRUE;

    /* Copy last item to current pointer */
    MEcopy(dut_t2_last_p,
	sizeof(DUT_T2_LDBLIST),
	(DUT_T2_LDBLIST *)dut_t2_p);
    /* decument ldb_num_entry in dut_f2_p */
    dut_f2_p->dut_f2_0ldb_num_entry--;
    /* Sort the LDB list array */
    dut_uu13_buble_sort(dut_cb->dut_form_ldblist->dut_f2_3ldblist,
		sizeof(DUT_T2_LDBLIST), DDB_MAXNAME, 
		dut_cb->dut_form_ldblist->dut_f2_0ldb_num_entry);
    return(E_DU_OK);
}


/*
** Name: dut_uu19_test_ldb - Test if a LDB is accessible.
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
**      22-mar-1989 (alexc)
**          Initial Creation.
**      24-oct-1994 (newca01) 
**          Added code to call dut_uu26_retry_dircont
*/

DUT_STATUS
dut_uu19_test_ldb(dut_cb, dut_errcb, dut_t2_p)
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
DUT_T2_LDBLIST	*dut_t2_p;
{
    EXEC SQL begin declare section;
    char		dut_buffer[DB_LONGMSG];
    DUT_T2_LDBLIST	*dut_t2_tmp;
    EXEC SQL end declare section;
    i4			dut_t2_cnt;
    i4			dut_t2_i;

    dut_cb->dut_on_error_noop = TRUE;
    dut_cb->dut_on_error_exit = FALSE;

    dut_t2_tmp = dut_cb->dut_form_ldblist->dut_f2_3ldblist;
    dut_t2_cnt = dut_cb->dut_form_ldblist->dut_f2_0ldb_num_entry;

    if (STcompare(dut_t2_p->dut_t2_1status, "?"))
    {
	return(E_DU_OK);
    }

    STcopy(dut_t2_p->dut_t2_3ldbname, dut_cb->dut_c3_ldbname);
    STcopy(dut_t2_p->dut_t2_2nodename, dut_cb->dut_c4_ldbnode);

    STprintf(dut_buffer, DUT_M49_TEST_DB,
		dut_t2_p->dut_t2_2nodename,
		dut_t2_p->dut_t2_3ldbname);
    EXEC FRS message :dut_buffer;

    if (dut_uu2_dbop(DUT_DIRECT_CONNECT, dut_cb, dut_errcb) == E_DU_OK)
    {
	STcopy(DUT_M50_UP, dut_t2_p->dut_t2_1status);
	if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
	{
	    dut_ee1_error(dut_errcb, 0/* STARVIEW FATAL ERROR */, 0);
	}
    }
    else
    {
	if (dut_uu26_retry_dircont(dut_cb, dut_errcb) == E_DU_OK)
	    STcopy(DUT_M50_UP, dut_t2_p->dut_t2_1status);
	else
	    STcopy(DUT_M51_DOWN, dut_t2_p->dut_t2_1status);
    }

    dut_cb->dut_on_error_noop = FALSE;

    for (dut_t2_i=0; dut_t2_i < dut_t2_cnt; dut_t2_i++, dut_t2_tmp++)
    {
	if (!STcompare(dut_t2_p->dut_t2_3ldbname,
			dut_t2_tmp->dut_t2_3ldbname)
		&&
	    !STcompare(dut_t2_p->dut_t2_2nodename,
			dut_t2_tmp->dut_t2_2nodename))
	{
	    STcopy(dut_t2_p->dut_t2_1status, dut_t2_tmp->dut_t2_1status);
	    break;
	}
    }

    dut_ff2_4reload_ldb(dut_cb, dut_errcb);

    EXEC FRS redisplay;
    return(E_DU_OK);
}


/*
** Name: dut_uu20_test_nodelist - Test a list of nodes for accessibility.
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
**      22-mar-1989 (alexc)
**          Initial Creation.
*/

void
dut_uu20_test_nodelist(dut_cb, dut_errcb)
DUT_CB	*dut_cb;
DUT_ERRCB	*dut_errcb;
{
    DUT_F2_LDBLIST	*dut_f2_p;
    DUT_T2_LDBLIST	*dut_t2_p;
    DUT_T2_LDBLIST	*dut_t2_tmp;
    DUT_T2_LDBLIST	*dut_t2_curr_p;
    i4			dut_t2_count;
    i4			dut_i;
    i4			dut_j;

    dut_f2_p = dut_cb->dut_form_ldblist;
    dut_t2_p = dut_f2_p->dut_f2_3ldblist;
    dut_t2_curr_p = dut_f2_p->dut_f2_3ldblist_curr_p;
    dut_t2_count = dut_f2_p->dut_f2_0ldb_num_entry;

    dut_uu23_reset_ldblist(dut_cb, dut_errcb);
    for (dut_i = 0; dut_i < dut_t2_count; dut_i++)
    {
	if (!STcompare(dut_t2_p->dut_t2_1status, "?"))
	{
	    dut_uu21_test_node(dut_cb, dut_errcb, dut_t2_p->dut_t2_2nodename);
	}
	/* This for loop will set the rest of the LDBs with the same nodename
	** with the same value in its STATUS field.
	*/
	for (dut_j = dut_i, dut_t2_tmp = dut_t2_p;
		dut_j < dut_t2_count;
		dut_j++, dut_t2_tmp++)
	{
	    if (!STcompare(dut_t2_p->dut_t2_2nodename,
				dut_t2_tmp->dut_t2_2nodename))
	    {
		STcopy(dut_t2_p->dut_t2_1status, dut_t2_tmp->dut_t2_1status);
	    }
	}
	dut_t2_p++;
    }
}


/*
** Name: dut_uu21_test_node - Test a node for accessibility.
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
**      22-mar-1989 (alexc)
**          Initial Creation.
**      24-oct-1994 (newca01)
**          Added code to call new dut_uu26_retry_dircont
*/
DUT_STATUS
dut_uu21_test_node(dut_cb, dut_errcb, dut_nodename)
DUT_CB	*dut_cb;
DUT_ERRCB	*dut_errcb;
char		*dut_nodename;
{
    EXEC SQL begin declare section;
    char		dut_buffer[DB_LONGMSG];
    EXEC SQL end declare section;
    DUT_F2_LDBLIST	*dut_f2_p;
    DUT_T2_LDBLIST	*dut_t2_p;
    DUT_T2_LDBLIST	*dut_t2_tmp;
    DUT_T2_LDBLIST	*dut_t2_curr_p;
    i4			dut_t2_count;
    i4			dut_i;
    i4			dut_j;
    char		dut_status[DDB_MAXNAME];

    dut_f2_p = dut_cb->dut_form_ldblist;
    dut_t2_p = dut_f2_p->dut_f2_3ldblist;
    dut_t2_curr_p = dut_f2_p->dut_f2_3ldblist_curr_p;
    dut_t2_count = dut_f2_p->dut_f2_0ldb_num_entry;

    STcopy("iidbdb", dut_cb->dut_c3_ldbname);
    STcopy(dut_nodename, dut_cb->dut_c4_ldbnode);

    STprintf(dut_buffer, DUT_M52_TEST_NODE, dut_nodename);
    EXEC FRS message :dut_buffer;

    if (dut_uu2_dbop(DUT_DIRECT_CONNECT, dut_cb, dut_errcb) == E_DU_OK)
    {
	STcopy(DUT_M50_UP, dut_status);
    }
    else
    {
	if (dut_uu26_retry_dircont(dut_cb,dut_errcb) == E_DU_OK)
	    STcopy(DUT_M50_UP, dut_status);
        else 
	    STcopy(DUT_M51_DOWN, dut_status);
    }
    for (dut_j = 0, dut_t2_tmp = dut_t2_p;
	dut_j < dut_t2_count;
	dut_j++, dut_t2_tmp++)
    {
	if (!STcompare(dut_nodename,
			dut_t2_tmp->dut_t2_2nodename))
	{
	    STcopy(dut_status, dut_t2_tmp->dut_t2_1status);
	    dut_ff2_4reload_ldb(dut_cb, dut_errcb);
	    EXEC FRS redisplay;
	}
    }
    if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU1809_SV_CANT_DIR_DISCONNECT, 0);
    }
    return(E_DU_OK);
}


/*
** Name: dut_uu22_test_ldblist - Test a list of LDBs for accessibility.
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
**      22-mar-1989 (alexc)
**          Initial Creation.
*/

void
dut_uu22_test_ldblist(dut_cb, dut_errcb)
DUT_CB	*dut_cb;
DUT_ERRCB	*dut_errcb;
{
    DUT_F2_LDBLIST	*dut_f2_p;
    DUT_T2_LDBLIST	*dut_t2_p;
    DUT_T2_LDBLIST	*dut_t2_curr_p;
    i4			dut_t2_count;
    i4			dut_i;

    dut_f2_p = dut_cb->dut_form_ldblist;
    dut_t2_p = dut_f2_p->dut_f2_3ldblist;
    dut_t2_count = dut_f2_p->dut_f2_0ldb_num_entry;

    dut_uu23_reset_ldblist(dut_cb, dut_errcb);
    for (dut_i=0; dut_i < dut_t2_count; dut_i++, dut_t2_p++)
    {
	dut_uu19_test_ldb(dut_cb, dut_errcb, dut_t2_p);
	dut_ff2_4reload_ldb(dut_cb, dut_errcb);
	EXEC FRS redisplay;
    }
}


/*
** Name: dut_uu23_reset_ldblist - Test a node for accessibility.
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
**      22-mar-1989 (alexc)
**          Initial Creation.
*/
void
dut_uu23_reset_ldblist(dut_cb, dut_errcb)
DUT_CB	*dut_cb;
DUT_ERRCB	*dut_errcb;
{
    DUT_F2_LDBLIST	*dut_f2_p;
    DUT_T2_LDBLIST	*dut_t2_p;
    DUT_T2_LDBLIST	*dut_t2_curr_p;
    i4			dut_t2_count;
    i4			dut_i;

    dut_f2_p = dut_cb->dut_form_ldblist;
    dut_t2_p = dut_f2_p->dut_f2_3ldblist;
    dut_t2_count = dut_f2_p->dut_f2_0ldb_num_entry;

    for (dut_i=0; dut_i < dut_t2_count; dut_i++, dut_t2_p++)
    {
	STcopy("?", dut_t2_p->dut_t2_1status);
    }
}


/*
** Name: dut_uu24_gcf_node - get node names from GCF files.
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
**      22-mar-1989 (alexc)
**          Initial Creation.
*/
DUT_STATUS
dut_uu24_gcf_node(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    DUT_P14_NODE	*dut_p14_node_p;
    DUT_T14_NODE	*dut_t14_node_p;
    DUT_T14_NODE	*dut_t14_node_curr_p;
    i4			dut_p14_0count;
    EXEC SQL end declare section;
    i4			dut_p14_0fill;

    dut_p14_0count = 0;
    dut_p14_0fill  = 0;
    dut_p14_node_p = dut_cb->dut_popup_b_node;
    dut_t14_node_p = dut_p14_node_p->dut_p14_1node;
    dut_t14_node_curr_p = dut_p14_node_p->dut_p14_1node_curr_p;

    if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU1805_SV_CANT_OPEN_DDB, 2, 0, 
			dut_cb->dut_c1_ddbname);
	return(DUT_ERROR);
    }

    EXEC SQL select distinct
	count(*) into :dut_p14_0count
	from iiddb_ldbids;

    if (dut_p14_0count > dut_p14_node_p->dut_p14_0size)
    {
	MEfree(dut_p14_node_p->dut_p14_1node);
	dut_p14_node_p->dut_p14_1node = NULL;
	dut_t14_node_p = NULL;
    }

    if (dut_t14_node_p == NULL)
    {
	dut_t14_node_p = (DUT_T14_NODE *)MEreqmem(0,
		(dut_p14_0count + DUT_ME_EXTRA) * sizeof(DUT_T14_NODE),
		TRUE, NULL);
	dut_p14_node_p->dut_p14_1node = dut_t14_node_p;
	dut_p14_node_p->dut_p14_0size = (dut_p14_0count + DUT_ME_EXTRA);
    }

    dut_t14_node_curr_p = dut_t14_node_p;

    EXEC SQL
	SELECT distinct
	    trim(ldb_node)
	into
	    :dut_t14_node_curr_p->dut_t14_1node
	from
	    iiddb_ldbids
	order by 1;
    EXEC SQL begin;
	dut_p14_0fill++;
	dut_t14_node_curr_p++;
	if (sqlca.sqlcode < 0)
	{
	    dut_cb->dut_c8_status = DUT_ERROR;
	    EXEC SQL endselect;
	}
    EXEC SQL end;
    dut_p14_node_p->dut_p14_0num_entry = dut_p14_0fill;

    return(E_DU_OK);
}


/*
** Name: dut_uu25_insert_node - inserts a node into the node list.
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
**      22-mar-1989 (alexc)
**          Initial Creation.
*/

DUT_STATUS
dut_uu25_insert_node(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    DUT_P14_NODE	*dut_p14_node_p;
    DUT_T14_NODE	*dut_t14_tmp_p;
    DUT_T14_NODE	*dut_t14_node_p;
    DUT_T14_NODE	*dut_t14_node_curr_p;
    i4			dut_p14_0count;
    EXEC SQL end declare section;
    i4			found = FALSE;

    dut_p14_node_p = dut_cb->dut_popup_b_node;
    dut_t14_node_p = dut_p14_node_p->dut_p14_1node;
    dut_t14_node_curr_p = dut_p14_node_p->dut_p14_1node_curr_p;
    dut_p14_0count = dut_p14_node_p->dut_p14_0num_entry+1;

    while (!found && dut_p14_0count--)
    {
	if (!STcompare(dut_t14_node_p->dut_t14_1node,
			dut_cb->dut_c16_browse_node))
	{
	    found = TRUE;
	}
	dut_t14_node_p++;
    }

    if (found == TRUE)
    {
	return(DUT_ERROR);
    }

    dut_t14_node_p = dut_p14_node_p->dut_p14_1node;
    dut_p14_0count = dut_p14_node_p->dut_p14_0num_entry+1;

    if (dut_p14_0count > dut_p14_node_p->dut_p14_0size)
    {
	dut_t14_tmp_p = dut_t14_node_p;
	dut_t14_node_p = (DUT_T14_NODE *)MEreqmem(0,
		(dut_p14_0count + DUT_ME_EXTRA) * sizeof(DUT_T14_NODE),
		TRUE, NULL);
	MEcopy(dut_t14_tmp_p,
		sizeof(DUT_T14_NODE) * dut_p14_node_p->dut_p14_0num_entry,
		dut_t14_node_p);
	MEfree(dut_t14_tmp_p);
	dut_p14_node_p->dut_p14_1node = dut_t14_node_p;
	dut_p14_node_p->dut_p14_0size = (dut_p14_0count + DUT_ME_EXTRA);
    }

    dut_t14_node_curr_p = dut_t14_node_p;
    dut_t14_tmp_p = (DUT_T14_NODE *)
			(dut_t14_node_p +
			dut_p14_node_p->dut_p14_0num_entry);

    STcopy(dut_cb->dut_c16_browse_node, dut_t14_tmp_p->dut_t14_1node);
    dut_uu13_buble_sort(dut_t14_node_p, sizeof(DUT_T14_NODE), DDB_MAXNAME,
			dut_p14_node_p->dut_p14_0num_entry);
    return(E_DU_OK);
}
/*
** Name: dut_uu26_retry_dircont - retry direct connect on gateways
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
**       21-oct 1994 (newca01)
**          Initial Creation.
*/
DUT_STATUS
dut_uu26_retry_dircont(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	*dut_cb;
DUT_ERRCB	*dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    char		dut_buffer[DDB_2MAXNAME];
    EXEC SQL end declare section;
    DUT_F2_LDBLIST	*dut_f2_p;
    DUT_T2_LDBLIST	*dut_t2_p;
    DUT_T2_LDBLIST	*dut_t2_tmp;
    DUT_T2_LDBLIST	*dut_t2_curr_p;
    i4			dut_t2_count;
    i4			dut_i;
    i4			dut_j;
    char		dut_status[DDB_MAXNAME];
    DUT_STATUS          ret_status;

    dut_f2_p = dut_cb->dut_form_ldblist;
    dut_t2_p = dut_f2_p->dut_f2_3ldblist;
    dut_t2_curr_p = dut_f2_p->dut_f2_3ldblist_curr_p;
    dut_t2_count = dut_f2_p->dut_f2_0ldb_num_entry;
    ret_status = DUT_ERROR;

    for (dut_j = 0, dut_t2_tmp = dut_t2_p;
	dut_j < dut_t2_count;
	dut_j++, dut_t2_tmp++)
    {
	if (!STcompare(dut_cb->dut_c4_ldbnode,
			dut_t2_tmp->dut_t2_2nodename))
	{
	    if (STcompare(dut_t2_tmp->dut_t2_4ldbtype, 
			       "INGRES")) 
            { 
		EXEC SQL direct connect 
			  with 
			      node=:dut_cb->dut_c4_ldbnode,
			      database=:dut_cb->dut_c3_ldbname, 
			      dbms=:dut_t2_tmp->dut_t2_4ldbtype;
                if ( sqlca.sqlcode == 0)
		{
		    STcopy(dut_cb->dut_c4_ldbnode, 
			   dut_cb->dut_c8_prev_ldbnode);
                    STcopy(dut_cb->dut_c3_ldbname,
			   dut_cb->dut_c9_prev_ldbname); 
                    dut_cb->dut_c7_status = DUT_S_DIRCONT_LDB;
		    ret_status = E_DU_OK;
		    break;
                }
            } 
        }
    }
    return(ret_status);
}





