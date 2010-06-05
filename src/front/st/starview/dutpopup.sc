/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <dbms.h>
#include    <er.h>
#include    <duerr.h>
#include    <me.h>
#include    <st.h>
#include    <nm.h>
#include    <cv.h>
#include    <pc.h>
EXEC SQL INCLUDE 'dut.sh';
EXEC SQL INCLUDE SQLCA;
#include    <cv.h>

#include    "duustrings.h"

/**
**
**  Name: DUTPOPUP.SC - Popup functions for StarView.
**
**  Description:

**	dut_pp2_1criteria - Popup for Distributed object selection
**			criteria.
**	dut_pp3_1register - ** Removed **
**	dut_pp4_1dblist - ** Removed **
**	dut_pp4_2load_dblist - ** Removed **
**	dut_pp5_1tablelist - ** Removed **
**	dut_pp5_2load_tablelist - ** Removed **
**	dut_pp6_1nodelist - Popup for Node listing of the current installation
**			or node listing all LDBs registered with a given
**			DDB.
**	dut_pp6_2load_nodelist - Function to load node information from
**			iistar_cdbs from CDB(list of CDB nodes).
**	dut_pp6_3load_nodelist - Function to load node information from
**			iiddb_ldbids(iidd_ddb_ldbids) the list of nodes
**			where local objects resides.
**	dut_pp7_1dbattr - Popup for displaying database attributes.
**	dut_pp7_2get_dbname - Get dbname from screen.
**	dut_pp7_3load_dbattr - Loads attributes from a given Database.
**	dut_pp8_1tableattr - Popup for displaying table attributes.
**	dut_pp9_1viewattr - Popup for displaying view attributes.
**	dut_pp10_1regattr - ** Removed **
**	dut_pp11_1perror - Popup used for debugging.
**	dut_pp12_1ldblist - Popup for listing of local databases for
**			select into the Register syntax popup.
**	dut_pp12_2load_ldblist - Function to load Local database names into
**			an array.
**	dut_pp13_1ownlist - Popup for list of owners of lcoal database(s).
**	dut_pp13_2load_ownlist - Function to load local object owner names
**			into an array.
**	dut_pp14_1browse - Popup for browsing nodes, databases and tables.
**	dut_pp14_1browse_node - Popup for browsing nodes, databases and tables.
**	dut_pp14_2load_node - Load node name into Popup.
**	dut_pp15_1ldb	- Browse local database name.
**	dut_pp16_1browse_table - browse local database name.
**	dut_pp16_2register - Register the table object.
**	dut_pp16_3browse_load_table - load information into the browse table.
**
**
**  History:
**      02-dec-1988 (alexc)
**          Creation.
**	08-mar-1989 (alexc)
**	    Alpha test version.
**	03-may-1989 (alexc)
**	    Added Browsing functions: dut_pp14...() to dut_pp16_2...().
**	11-jul-1989 (alexc)
**	    Removed functions: dut_pp3_1register(), dut_pp4_1dblist(),
**		dut_pp4_2load_dblist(), dut_pp5_1tablelist(),
**		dut_pp5_2load_tablelist(), dut_pp10_1regattr().
**		These functions are no longer used.
**	11-Aug-1989 (alexc)
**	    Added function dut_pp16_3browse_load_table() to reload
**		table field from database after SQL or TABLES.
**	25-Jul-1991 (fpang)
**	    1. dut_pp7_1dbattr() modified to recognize forced consistent
**	       databases.
**	    2. Return E_DU_OK in dut_pp16_3browse_load_table() if 
**	       successful.
**      12-sep-1991 (fpang)
**          1. Changed Star*View to StarView. -B39562.
**          2. Mapped FIND, TOP and BOTTOM to standard INGRES FRSKEYS. -B39661
**	26-Aug-1993 (fredv)
**		Included <me.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of cv.h and pc.h plus function return types
**	    to eliminate gcc 4.3 warnings.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**/


/*
** Name: dut_pp2_1criteria	- Distributed Object Selection Popup
**
** Description:
**	Uses FRS, and has the following options for the form DUT_P2_SELECT.
**	    'Select'	Sets the selection criteria in (PTR *)dut_f3_objlist_p.
**	    'NodeHelp'	List all nodes from iiddb_ldbids(iidd_ddb_ldbids).
**	    'LDBHelp'	List all LDBs that has objects register in current DDB.
**	    'OwnerHelp'	List all owners of DDB objects.
**	    'Help'	Help on form.
**	    'End'	Exit.
**
** Inputs:
**	dut_cb
**	dut_errcb
**
** Outputs:
**	dut_f3_objlist_p
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-jan-1989 (alexc)
**          Creation.
*/
void
dut_pp2_1criteria(dut_cb, dut_errcb, dut_f3_objlist_p)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
DUT_F3_OBJLIST	*dut_f3_objlist_p;
EXEC SQL END DECLARE SECTION;
{
    /* If connected to CDB, use a different cursor */

    EXEC FRS display dut_p2_select;

    EXEC FRS initialize;
    EXEC FRS begin;
	EXEC FRS putform dut_p2_select
	    (
	    dut_p2_1nodename	= :dut_f3_objlist_p->dut_f3_1nodename,
	    dut_p2_2dbname	= :dut_f3_objlist_p->dut_f3_2dbname,
	    dut_p2_3owner	= :dut_f3_objlist_p->dut_f3_3owner,
 	    dut_p2_5show_view	= :dut_f3_objlist_p->dut_f3_5show_view,
	    dut_p2_6show_table	= :dut_f3_objlist_p->dut_f3_6show_table,
	    dut_p2_7show_system	= :dut_f3_objlist_p->dut_f3_7show_system
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'Select', frskey4;
    EXEC FRS begin;
	EXEC FRS getform dut_p2_select
	    (
	    :dut_f3_objlist_p->dut_f3_1nodename:dut_cb->dut_null_1	
			= dut_p2_1nodename,
	    :dut_f3_objlist_p->dut_f3_2dbname:dut_cb->dut_null_2	
			= dut_p2_2dbname,
	    :dut_f3_objlist_p->dut_f3_3owner:dut_cb->dut_null_3
			= dut_p2_3owner,
	    :dut_f3_objlist_p->dut_f3_5show_view:dut_cb->dut_null_5
			= dut_p2_5show_view,
	    :dut_f3_objlist_p->dut_f3_6show_table:dut_cb->dut_null_6
			= dut_p2_6show_table,
	    :dut_f3_objlist_p->dut_f3_7show_system:dut_cb->dut_null_7
			= dut_p2_7show_system
	    );
	if (dut_cb->dut_null_1 == -1)
	{
	    dut_f3_objlist_p->dut_f3_1nodename[0] = EOS;
	}
	if (dut_cb->dut_null_2 == -1)
	{
	    dut_f3_objlist_p->dut_f3_2dbname[0] = EOS;
	}
	if (dut_cb->dut_null_3 == -1)
	{
	    dut_f3_objlist_p->dut_f3_3owner[0] = EOS;
	}
	if (dut_cb->dut_null_5 == -1)
	{
	    dut_f3_objlist_p->dut_f3_5show_view[0] = EOS;
	}
	if (dut_cb->dut_null_6 == -1)
	{
	    dut_f3_objlist_p->dut_f3_6show_table[0] = EOS;
	}
	if (dut_cb->dut_null_7 == -1)
	{
	    dut_f3_objlist_p->dut_f3_7show_system[0] = EOS;
	}
	if ((dut_f3_objlist_p->dut_f3_1nodename[0] == EOS)	||
	    (dut_f3_objlist_p->dut_f3_2dbname[0]   == EOS)	||
	    (dut_f3_objlist_p->dut_f3_3owner[0]   == EOS)	||
	    (dut_f3_objlist_p->dut_f3_5show_view[0] == EOS)	||
	    (dut_f3_objlist_p->dut_f3_6show_table[0] == EOS)	||
	    (dut_f3_objlist_p->dut_f3_7show_system[0] == EOS))
	{
	    dut_ee1_error(dut_errcb, W_DU1817_SV_ERROR_IN_CRITERIA, 0);
	}
	else
	{
	    dut_cb->dut_c8_status = DUT_S_NEW_OBJ_SELECT;
	    EXEC FRS breakdisplay;
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'NodeHelp';
    EXEC FRS begin;
	dut_cb->dut_c9_status = DUT_P_POPUP_2;
	dut_pp6_1nodelist(dut_cb, dut_errcb);
	dut_cb->dut_c9_status = DUT_F_FORM_3;
	if (dut_cb->dut_c0_buffer[0] != EOS)
	    STcopy(dut_cb->dut_c0_buffer, dut_f3_objlist_p->dut_f3_1nodename);
	EXEC FRS putform dut_p2_select
	    (
	    dut_p2_1nodename = :dut_f3_objlist_p->dut_f3_1nodename
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'LDBHelp';
    EXEC FRS begin;
	/* This will help user change selection criteria for Node Name
	   and Database Name.  This is the only way dut_f3_1nodename,
	   and dut_f3_2dbname can be changed. */
	dut_cb->dut_c9_status = DUT_P_POPUP_2;
	dut_pp12_1ldblist(dut_cb, dut_errcb);
	dut_cb->dut_c9_status = DUT_F_FORM_3;
	if (dut_cb->dut_c0_buffer[0] != EOS)
	    STcopy(dut_cb->dut_c0_buffer, dut_f3_objlist_p->dut_f3_2dbname);
	EXEC FRS putform dut_p2_select
	    (
	    dut_p2_2dbname = :dut_f3_objlist_p->dut_f3_2dbname
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'OwnerHelp';
    EXEC FRS begin;
	dut_cb->dut_c9_status = DUT_P_POPUP_2;
	dut_pp13_1ownlist(dut_cb, dut_errcb);
	dut_cb->dut_c9_status = DUT_F_FORM_3;
	if (dut_cb->dut_c0_buffer[0] != EOS)
	    STcopy(dut_cb->dut_c0_buffer, dut_f3_objlist_p->dut_f3_3owner);
	EXEC FRS putform dut_p2_select
	    (
	    dut_p2_3owner = :dut_f3_objlist_p->dut_f3_3owner
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'Help', frskey1;
    EXEC FRS begin;
	EXEC FRS HELP_FRS
	    (
	    subject	= 'Distributed Database Help',
	    file	= :dut_cb->dut_helpcb->dut_hp2_1name
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	EXEC FRS breakdisplay;
    EXEC FRS end;
}


/*
** Name: dut_pp6_1nodelist
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      2-dec-1988 (alexc)
**          Creation.
*/
void
dut_pp6_1nodelist(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    i4		n_rows;
    EXEC SQL END DECLARE SECTION;
    DUT_P6_NODELIST *dut_p6_nodelist_p;

    EXEC SQL whenever sqlerror call dut_ee5_none_starview;

    dut_p6_nodelist_p = dut_cb->dut_popup_nodelist;

    EXEC FRS inittable dut_p6_nodelist dut_p6_1nodelist read;
    EXEC FRS display dut_p6_nodelist;

    EXEC FRS initialize;
    EXEC FRS begin;
	dut_cb->dut_c0_buffer[0] = (char)EOS;
	if (dut_cb->dut_c9_status == DUT_F_FORM_1)
	{
	    EXEC FRS putform dut_p6_nodelist
		(
		dut_p6_0name = 'STAR installation.'
		);
	    dut_pp6_2load_nodelist(dut_cb, dut_errcb);
	}
	else
	{
	    dut_cb->dut_c8_status = E_DU_OK;
	    EXEC FRS putform dut_p6_nodelist
		(
		dut_p6_0name = 'Distributed Database.'
		);
	    dut_pp6_3load_nodelist(dut_cb, dut_errcb);
	    if (dut_cb->dut_c8_status == DUT_ERROR)
	    {
		EXEC FRS breakdisplay;
	    }
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'Select', frskey4;
    EXEC FRS begin;
	EXEC FRS inquire_frs table dut_p6_nodelist
	    (
	    :n_rows = lastrow(dut_p6_1nodelist)
	    );
	if (n_rows == 0)
	{
	    dut_cb->dut_c0_buffer2[0] = EOS;
	    EXEC FRS breakdisplay;
	}
	EXEC FRS getrow dut_p6_nodelist dut_p6_1nodelist
	    (
	    :dut_cb->dut_c0_buffer = dut_t6_1nodename
	    );
	EXEC FRS breakdisplay;
    EXEC FRS end;

    EXEC FRS activate menuitem 'Top', frskey5 ;
    EXEC FRS begin;
	dut_uu7_table_top(NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Bottom', frskey6 ;
    EXEC FRS begin;
	dut_uu8_table_bottom(NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Find', frskey7 ;
    EXEC FRS begin;
	dut_uu9_table_find(dut_cb, dut_errcb, DUT_M08_NODENAME, NULL, NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Help', frskey1;
    EXEC FRS begin;
	EXEC FRS HELP_FRS
	    (
	    subject	= 'Distributed Database Help',
	    file	= :dut_cb->dut_helpcb->dut_hp6_1name
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	EXEC FRS breakdisplay;
    EXEC FRS end;
}


/*
** Name: dut_pp6_2load_nodelist
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      2-dec-1988 (alexc)
**          Creation.
*/

DUT_STATUS
dut_pp6_2load_nodelist(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    DUT_P6_NODELIST	*dut_p6_nodelist_p;
    DUT_T6_NODELIST	*dut_t6_nodelist_p;
    DUT_T6_NODELIST	*dut_t6_nodelist_curr_p;
    i4			dut_p6_0count;
    EXEC SQL END DECLARE SECTION;
    i4			dut_p6_0fill;

    EXEC SQL whenever sqlerror call dut_ee5_none_starview;

    dut_p6_0count	= 0;
    dut_p6_0fill	= 0;
    dut_p6_nodelist_p = (DUT_P6_NODELIST *)dut_cb->dut_popup_nodelist;
    dut_t6_nodelist_p = 
	    (DUT_T6_NODELIST *)dut_cb->dut_popup_nodelist->dut_p6_1nodelist;

    dut_cb->dut_on_error_exit = FALSE;
    dut_cb->dut_on_error_noop = FALSE;

    STcopy(dut_cb->dut_c2_nodename, dut_cb->dut_c0_iidbdbnode);
    if (dut_uu2_dbop(DUT_IIDBDBOPEN, dut_cb, dut_errcb) 
	!= E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU180A_SV_CANT_OPEN_IIDBDB, 0);
	return(DUT_ERROR);
    }

    EXEC SQL select distinct
	count(*) into :dut_p6_0count
	from iistar_cdbs;

    if (dut_p6_0count > dut_p6_nodelist_p->dut_p6_0size)
    {
	MEfree(dut_p6_nodelist_p->dut_p6_1nodelist);
	dut_p6_nodelist_p->dut_p6_1nodelist = NULL;
	dut_t6_nodelist_p = NULL;
    }

    dut_cb->dut_on_error_exit = TRUE;
    if (dut_t6_nodelist_p == NULL)
    {
	dut_t6_nodelist_p = (DUT_T6_NODELIST *)MEreqmem(0,
		(dut_p6_0count + DUT_ME_EXTRA) * sizeof(DUT_T6_NODELIST),
		TRUE, NULL);
	dut_p6_nodelist_p->dut_p6_1nodelist
		= (DUT_T6_NODELIST *)dut_t6_nodelist_p;
	dut_p6_nodelist_p->dut_p6_0size = (dut_p6_0count + DUT_ME_EXTRA);
    }

    dut_t6_nodelist_curr_p = (DUT_T6_NODELIST *)dut_t6_nodelist_p;

    dut_cb->dut_on_error_exit = TRUE;

    EXEC SQL
	SELECT	distinct
	    trim(cdb_node)
	into
	    :dut_t6_nodelist_p->dut_t6_1nodename
	from iistar_cdbs
	order by 1;
    EXEC SQL begin;
	EXEC FRS loadtable dut_p6_nodelist dut_p6_1nodelist
	    (
	    dut_t6_1nodename = :dut_t6_nodelist_p->dut_t6_1nodename
	    );
	
	dut_p6_0fill++;
	dut_t6_nodelist_p++;
	if (sqlca.sqlcode < 0)
	{
	    dut_cb->dut_c8_status = DUT_ERROR;
	    EXEC SQL endselect;
	}
    EXEC SQL end;
    dut_cb->dut_on_error_exit = FALSE;
    dut_p6_nodelist_p->dut_p6_0num_entry = dut_p6_0fill;

}


/*
** Name: dut_pp6_3load_nodelist
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      2-dec-1988 (alexc)
**          Creation.
*/
DUT_STATUS
dut_pp6_3load_nodelist(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    DUT_P6_NODELIST	*dut_p6_nodelist_p;
    DUT_T6_NODELIST	*dut_t6_nodelist_p;
    DUT_T6_NODELIST	*dut_t6_nodelist_curr_p;
    i4			dut_p6_0count;
    EXEC SQL END DECLARE SECTION;
    i4			dut_p6_0fill;

    EXEC SQL whenever sqlerror call dut_ee5_none_starview;

    dut_p6_0count	= 0;
    dut_p6_0fill	= 0;
    dut_p6_nodelist_p = (DUT_P6_NODELIST *)dut_cb->dut_popup_nodelist;
    dut_t6_nodelist_p =
	    (DUT_T6_NODELIST *)dut_cb->dut_popup_nodelist->dut_p6_1nodelist;

    if (dut_uu2_dbop(DUT_DIRECT_CONNECT_CDB, dut_cb, dut_errcb) 
	!= E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU1806_SV_CANT_DIRECT_CONNECT, 2,
		0, DUT_1CDB_LONG);
	return(DUT_ERROR);
    }

    EXEC SQL select distinct
	count(*) into :dut_p6_0count
	from iidd_ddb_ldbids;

    if (dut_p6_0count > dut_p6_nodelist_p->dut_p6_0size)
    {
	MEfree(dut_p6_nodelist_p->dut_p6_1nodelist);
	dut_p6_nodelist_p->dut_p6_1nodelist = NULL;
	dut_t6_nodelist_p = NULL;
    }

    if (dut_t6_nodelist_p == NULL)
    {
	dut_t6_nodelist_p = (DUT_T6_NODELIST *)MEreqmem(0,
		(dut_p6_0count + DUT_ME_EXTRA) * sizeof(DUT_T6_NODELIST),
		TRUE, NULL);
	dut_p6_nodelist_p->dut_p6_1nodelist =
		(DUT_T6_NODELIST *)dut_t6_nodelist_p;
	dut_p6_nodelist_p->dut_p6_0size = (dut_p6_0count + DUT_ME_EXTRA);
    }

    dut_t6_nodelist_curr_p = (DUT_T6_NODELIST *)dut_t6_nodelist_p;

    if (dut_cb->dut_c9_status == DUT_P_POPUP_2)
    {
	EXEC FRS loadtable dut_p6_nodelist dut_p6_1nodelist
	    (
	    dut_t6_1nodename = '*'
	    );
    }
	
    dut_cb->dut_on_error_exit = TRUE;

    EXEC SQL
	SELECT	distinct
	    trim(ldb_node)
	into
	    :dut_t6_nodelist_p->dut_t6_1nodename
	from iidd_ddb_ldbids
	order by 1;
    EXEC SQL begin;
	EXEC FRS loadtable dut_p6_nodelist dut_p6_1nodelist
	    (
	    dut_t6_1nodename = :dut_t6_nodelist_p->dut_t6_1nodename
	    );
	
	dut_p6_0fill++;
	dut_t6_nodelist_p++;
	if (sqlca.sqlcode < 0)
	{
	    dut_cb->dut_c8_status = DUT_ERROR;
	    EXEC SQL endselect;
	}
    EXEC SQL end;

    dut_cb->dut_on_error_exit = FALSE;

    dut_p6_nodelist_p->dut_p6_0num_entry = dut_p6_0fill;

}


/*
** Name: dut_pp7_1dbattr
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-jan-1989 (alexc)
**          Creation.
**	25-jul-1991 (fpang)
**	    Recognize forced consistent databases.
**      14-Feb-2008 (horda03) Bug 119914
**          Disconnect the IIDBDB connection once details
**          displayed. Leaving the connection results in ISQL
**          connecting to teh IIDBDB and not the Star DB.
*/

DUT_STATUS
dut_pp7_1dbattr(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    DUT_P7_DBATTR	*dut_p7_dbattr_p;
    i4			dut_count;
    i4                  open_iidbdb = FALSE;
    EXEC SQL end declare section;

    dut_p7_dbattr_p = dut_cb->dut_popup_dbattr;

    if (dut_p7_dbattr_p->dut_p7_0nodename[0] == EOS)
    {
	STcopy(dut_cb->dut_c0_iidbdbnode, 
    		dut_p7_dbattr_p->dut_p7_0nodename);
    }

    if (dut_p7_dbattr_p->dut_p7_1dbname[0] == EOS)
    {
	dut_pp7_2get_dbname(dut_cb, dut_errcb);
    }

    if (!STcompare(dut_p7_dbattr_p->dut_p7_0nodename, 
		    dut_cb->dut_c0_iidbdbnode))
    {
	if (dut_uu2_dbop(DUT_IIDBDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
	{
	    dut_ee1_error(dut_errcb, E_DU5111_SV_CANT_OPEN_IIDBDB, 0);
	    return(DUT_ERROR);
	}

        open_iidbdb = TRUE;
    }
    else
    {
	/* Direct connect to remote node for DBattr */
	STcopy(dut_p7_dbattr_p->dut_p7_0nodename, dut_cb->dut_c4_ldbnode);
	STcopy("iidbdb", dut_cb->dut_c3_ldbname);
	if (dut_uu2_dbop(DUT_DIRECT_CONNECT, dut_cb, dut_errcb) != E_DU_OK)
	{
	    dut_ee1_error(dut_errcb, W_DU1806_SV_CANT_DIRECT_CONNECT, 2, 0, 
			  dut_cb->dut_c3_ldbname);
	    return(DUT_ERROR);
	}
    }

    EXEC FRS display dut_p7_dbattr;

    EXEC FRS initialize;
    EXEC FRS begin;

	EXEC SQL select
	    count(name)
	INTO
	    :dut_count
	FROM	
	    iidatabase
	WHERE	
	    iidatabase.name = :dut_p7_dbattr_p->dut_p7_1dbname;

	if (dut_count == 0)
	{
	    /* Local database does not exist */
	    dut_ee1_error(dut_errcb, W_DU1818_SV_DB_DOES_NOT_EXIST, 0);
	    EXEC FRS breakdisplay;
	}

	EXEC SQL select
	    own,
	    dbdev,
	    ckpdev,
	    jnldev,
	    sortdev,
	    access,
	    dbservice
	INTO
	    :dut_p7_dbattr_p->dut_p7_2own,
	    :dut_p7_dbattr_p->dut_p7_3dbdev,
	    :dut_p7_dbattr_p->dut_p7_4ckpdev,
	    :dut_p7_dbattr_p->dut_p7_5jnldev,
	    :dut_p7_dbattr_p->dut_p7_6sortdev,
	    :dut_p7_dbattr_p->dut_p7_0access,
	    :dut_p7_dbattr_p->dut_p7_0dbservice
	FROM	
	    iidatabase
	WHERE	
	    iidatabase.name = :dut_p7_dbattr_p->dut_p7_1dbname;

	if (sqlca.sqlcode < 0)
	{
	    /* Database does not exist */
	    dut_ee1_error(dut_errcb, W_DU1818_SV_DB_DOES_NOT_EXIST, 0);
	    return(DUT_ERROR);
	}
	STprintf(dut_p7_dbattr_p->dut_p7_7access,
		DUT_M09_UNKNOWN_ACCESS,
		dut_p7_dbattr_p->dut_p7_0access);
	if (dut_p7_dbattr_p->dut_p7_0access & DUT_GLOBAL)
	{
	}
	if (dut_p7_dbattr_p->dut_p7_0access & DUT_RES_DDB)
	{
	    /* Never being use */
	    STcopy(DUT_M10_DB_IS_DDB,
			dut_p7_dbattr_p->dut_p7_7access);
	}
	if (dut_p7_dbattr_p->dut_p7_0access & DUT_DESTROYDB)
	{
	    STcopy(DUT_M11_DB_IS_DESTROY,
			dut_p7_dbattr_p->dut_p7_7access);
	}
	if (dut_p7_dbattr_p->dut_p7_0access & DUT_CREATEDB)
	{
	    STcopy(DUT_M12_DB_IS_CREATE,
			dut_p7_dbattr_p->dut_p7_7access);
	}
	if (dut_p7_dbattr_p->dut_p7_0access & DUT_OPERATIVE)
	{
	    STcopy(DUT_M13_DB_IS_OPERATE,
			dut_p7_dbattr_p->dut_p7_7access);
	}
	if (dut_p7_dbattr_p->dut_p7_0access & DUT_CONVERTING)
	{
	    STcopy(DUT_M14_DB_IS_CONVERTING,
			dut_p7_dbattr_p->dut_p7_7access);
	}

	switch (dut_p7_dbattr_p->dut_p7_0dbservice)
	{
	case DUT_LDB_TYPE:
	    /* Local database */
	    STcopy(DUT_M15_LDB,
			dut_p7_dbattr_p->dut_p7_8dbservice);
	    break;
	case DUT_DDB_TYPE:
	    /* Distributed database */
	    STcopy(DUT_M16_DDB,
			dut_p7_dbattr_p->dut_p7_8dbservice);
	    break;
	case DUT_CDB_TYPE:
	    /* Coordinator database of a Distributed database */
	    STcopy(DUT_M17_CDB,
			dut_p7_dbattr_p->dut_p7_8dbservice);
	    break;
	case DUT_RMS_TYPE:
	    /* RMS database */
	    STcopy(DUT_M54_RMS,
			dut_p7_dbattr_p->dut_p7_8dbservice);
	    break;
	case DUT_FRCD_CNST_TYPE:
	    /* Forced consistent database */
	    STcopy(DUT_M55_FRCD_CNST,
			dut_p7_dbattr_p->dut_p7_8dbservice);
	    break;
	default:
	    STprintf(dut_p7_dbattr_p->dut_p7_8dbservice,
			DUT_M18_UNKNOWN_DBSERV,
			dut_p7_dbattr_p->dut_p7_0dbservice);
	    break;
	}

	EXEC FRS putform dut_p7_dbattr
	    (
	    dut_p7_0nodename	= :dut_p7_dbattr_p->dut_p7_0nodename,
	    dut_p7_1dbname	= :dut_p7_dbattr_p->dut_p7_1dbname,
	    dut_p7_2own		= :dut_p7_dbattr_p->dut_p7_2own,
	    dut_p7_3dbdev	= :dut_p7_dbattr_p->dut_p7_3dbdev,
	    dut_p7_4ckpdev	= :dut_p7_dbattr_p->dut_p7_4ckpdev,
	    dut_p7_5jnldev	= :dut_p7_dbattr_p->dut_p7_5jnldev,
	    dut_p7_6sortdev	= :dut_p7_dbattr_p->dut_p7_6sortdev,
	    dut_p7_7access	= :dut_p7_dbattr_p->dut_p7_7access,
	    dut_p7_8dbservice	= :dut_p7_dbattr_p->dut_p7_8dbservice
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	EXEC FRS breakdisplay;
    EXEC FRS end;

    if (open_iidbdb)
    {
        if (dut_uu2_dbop(DUT_IIDBDBCLOSE, dut_cb, dut_errcb) != E_DU_OK)
        {
            return(DUT_ERROR);
        }
    }
    else if (dut_uu2_dbop(DUT_DIRECT_DISCONNECT, dut_cb, dut_errcb) != E_DU_OK)
    {
         return(DUT_ERROR);
    }
    
}


/*
** Name: dut_pp7_2get_dbname
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-jan-1989 (alexc)
**          Creation.
*/

void
dut_pp7_2get_dbname(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    DUT_P7_DBATTR	*dut_p7_dbattr_p;
    EXEC SQL end declare section;

    dut_p7_dbattr_p = dut_cb->dut_popup_dbattr;

    EXEC FRS display dut_p7_1dbname;

    EXEC FRS initialize;
    EXEC FRS begin;
	EXEC FRS putform dut_p7_1dbname
	(
	dut_p7_0nodename = :dut_cb->dut_c6_cdbnode,
	dut_p7_1dbname	 = :dut_cb->dut_c5_cdbname
	);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Go', frskey4 ;
    EXEC FRS begin;
	EXEC FRS getform dut_p7_1dbname
	(
	:dut_p7_dbattr_p->dut_p7_0nodename	= dut_p7_0nodename,
	:dut_p7_dbattr_p->dut_p7_1dbname	= dut_p7_1dbname
	);
	EXEC FRS breakdisplay;
    EXEC FRS end;

    EXEC FRS activate menuitem 'Help', frskey1;
    EXEC FRS begin;
	EXEC FRS HELP_FRS
	    (
	    subject	= 'Distributed Database Help',
	    file	= :dut_cb->dut_helpcb->dut_hp7_2name
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	EXEC FRS breakdisplay;
    EXEC FRS end;
}

/*
** Name: dut_pp7_3load_dbattr
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-jan-1989 (alexc)
**          Creation.
*/

DUT_STATUS
dut_pp7_3load_dbattr(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    DUT_P7_DBATTR	*dut_p7_dbattr_p;
    EXEC SQL end declare section;

    dut_p7_dbattr_p = dut_cb->dut_popup_dbattr;

    if (dut_p7_dbattr_p->dut_p7_1dbname[0] == EOS)
    {
	return(DUT_ERROR);
    }
    else
    {
	STcopy(dut_cb->dut_c2_nodename, dut_cb->dut_c0_iidbdbnode);
	if (dut_uu2_dbop(DUT_IIDBDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
        {
	    dut_ee1_error(dut_errcb, E_DU5111_SV_CANT_OPEN_IIDBDB, 0);
	    return(DUT_ERROR);
	}

	EXEC SQL repeat select
	    own,
	    dbdev,
	    ckpdev,
	    jnldev,
	    sortdev,
	    access,
	    dbservice
	into
	    :dut_p7_dbattr_p->dut_p7_2own,
	    :dut_p7_dbattr_p->dut_p7_3dbdev,
	    :dut_p7_dbattr_p->dut_p7_4ckpdev,
	    :dut_p7_dbattr_p->dut_p7_5jnldev,
	    :dut_p7_dbattr_p->dut_p7_6sortdev,
	    :dut_p7_dbattr_p->dut_p7_0access,
	    :dut_p7_dbattr_p->dut_p7_0dbservice
        from
	    iidbdb
	where
	    iidatabase.name = :dut_p7_dbattr_p->dut_p7_1dbname;

	switch (dut_p7_dbattr_p->dut_p7_0access)
	{
	}

	switch (dut_p7_dbattr_p->dut_p7_0dbservice)
	{
	case 0:
	    STcopy(DUT_M19_LDB_OR_CDB,
		dut_p7_dbattr_p->dut_p7_8dbservice);
	    break;
	case 1:
	    STcopy(DUT_M16_DDB,
		dut_p7_dbattr_p->dut_p7_8dbservice);
	    break;
	default:
	    STcopy(DUT_M20_UNKNOWN,
		dut_p7_dbattr_p->dut_p7_8dbservice);
	    break;
	}

	EXEC FRS putform dut_p7_dbattr
	    (
	    dut_p7_0nodename	= :dut_p7_dbattr_p->dut_p7_0nodename,
	    dut_p7_1dbname	= :dut_p7_dbattr_p->dut_p7_1dbname,
	    dut_p7_2own		= :dut_p7_dbattr_p->dut_p7_2own,
	    dut_p7_3dbdev	= :dut_p7_dbattr_p->dut_p7_3dbdev,
	    dut_p7_4ckpdev	= :dut_p7_dbattr_p->dut_p7_4ckpdev,
	    dut_p7_5jnldev	= :dut_p7_dbattr_p->dut_p7_5jnldev,
	    dut_p7_6sortdev	= :dut_p7_dbattr_p->dut_p7_6sortdev,
	    dut_p7_7access	= :dut_p7_dbattr_p->dut_p7_7access,
	    dut_p7_8dbservice	= :dut_p7_dbattr_p->dut_p7_8dbservice
	    );
    }
}

/*
** Name: dut_pp8_1objattr
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-jan-1989 (alexc)
**          Creation.
*/

DUT_STATUS
dut_pp8_1objattr(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    DUT_T3_OBJLIST	*dut_t3_objlist_p;
    DUT_P8_OBJATTR	*dut_p8_objattr_p;
    EXEC SQL end declare section;
    

    dut_t3_objlist_p	= dut_cb->dut_form_objlist->dut_f3_9objlist_curr_p;
    dut_p8_objattr_p	= dut_cb->dut_popup_objattr;

    /* Get Table information */

    if (dut_uu2_dbop(DUT_DIRECT_CONNECT_CDB, dut_cb, dut_errcb) 
	!= E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU1806_SV_CANT_DIRECT_CONNECT, 2,
		0, DUT_1CDB_LONG);
	return(DUT_ERROR);
    }

    EXEC SQL select
	iidd_tables.create_date,
	iidd_tables.alter_date,
	iidd_tables.table_type,
	iidd_tables.table_subtype,
	iidd_tables.table_version,
	iidd_tables.system_use
    into
	:dut_p8_objattr_p->dut_p8_4create_date,
	:dut_p8_objattr_p->dut_p8_5alter_date,
	:dut_p8_objattr_p->dut_p8_1objtype,
	:dut_p8_objattr_p->dut_p8_6objsubtype,
	:dut_p8_objattr_p->dut_p8_7objversion,
	:dut_p8_objattr_p->dut_p8_8system_use
    from
	iidd_tables
    where
	iidd_tables.table_name
			= :dut_p8_objattr_p->dut_p8_2objname
		and
	iidd_tables.table_owner
			= :dut_p8_objattr_p->dut_p8_3objowner;

    if (sqlca.sqlcode < 0)
    {
	/* Error, object does not exist */
	dut_cb->dut_c8_status = DUT_ERROR;
	return(DUT_ERROR);
    }

    EXEC SQL select
	iidd_ddb_objects.object_name,
	iidd_ddb_tableinfo.table_name,
	iidd_ddb_ldbids.ldb_dbms,
	iidd_ddb_ldbids.ldb_node,
	iidd_ddb_ldbids.ldb_database
    into
	:dut_p8_objattr_p->dut_p8_2objname,
	:dut_p8_objattr_p->dut_p8_9local_name,
	:dut_p8_objattr_p->dut_p8_10dbmstype,
	:dut_p8_objattr_p->dut_p8_11nodename,
	:dut_p8_objattr_p->dut_p8_12ldbname
    from
	iidd_ddb_objects, iidd_ddb_ldbids, iidd_ddb_tableinfo
    where
	iidd_ddb_objects.object_name 
			= :dut_p8_objattr_p->dut_p8_2objname
		and
	iidd_ddb_objects.object_owner 
			= :dut_p8_objattr_p->dut_p8_3objowner
	        and
	iidd_ddb_objects.object_base 
			= iidd_ddb_tableinfo.object_base
		and
	iidd_ddb_objects.object_index
			= iidd_ddb_tableinfo.object_index
		and
	iidd_ddb_tableinfo.ldb_id 
			= iidd_ddb_ldbids.ldb_id;
    if (sqlca.sqlcode < 0)
    {
	/* Error, object does not exist */
	dut_cb->dut_c8_status = DUT_ERROR;
	return(DUT_ERROR);
    }

    STtrmwhite(dut_p8_objattr_p->dut_p8_1objtype);
    STtrmwhite(dut_p8_objattr_p->dut_p8_8system_use);
    STtrmwhite(dut_p8_objattr_p->dut_p8_6objsubtype);

    switch(dut_p8_objattr_p->dut_p8_6objsubtype[0])
    {
	case 'L':
		switch(dut_p8_objattr_p->dut_p8_1objtype[0])
		{
			case 'T':
				STcopy(DUT_M21_REGTABLE,
					dut_p8_objattr_p->dut_p8_1objtype);
				STcopy(DUT_M22_TABLE,
					dut_p8_objattr_p->dut_p8_1objtype);
				break;
			case 'V':
				STcopy(DUT_M23_REGVIEW,
					dut_p8_objattr_p->dut_p8_1objtype);
				STcopy(DUT_M24_VIEW,
					dut_p8_objattr_p->dut_p8_1objtype);
				break;
			case 'I':
				STcopy(DUT_M25_REGINDEX,
					dut_p8_objattr_p->dut_p8_1objtype);
				STcopy(DUT_M26_INDEX,
					dut_p8_objattr_p->dut_p8_1objtype);
				break;
		}
		break;
	case 'N':
		switch(dut_p8_objattr_p->dut_p8_1objtype[0])
		{
			case 'T':
				STcopy(DUT_M27_STAR_TABLE,
					dut_p8_objattr_p->dut_p8_1objtype);
				STcopy(DUT_M22_TABLE,
					dut_p8_objattr_p->dut_p8_1objtype);
				break;
			case 'V':
				STcopy(DUT_M28_DIST_VIEW,
					dut_p8_objattr_p->dut_p8_1objtype);
				STcopy(DUT_M24_VIEW,
					dut_p8_objattr_p->dut_p8_1objtype);
				break;
			case 'P':
				STcopy(DUT_M29_PART_TABLE,
					dut_p8_objattr_p->dut_p8_1objtype);
				break;
			case 'R':
				STcopy(DUT_M30_REPLICATE_TABLE,
					dut_p8_objattr_p->dut_p8_1objtype);
				break;
			case 'I':
				STcopy(DUT_M31_NATIVE_INDEX,
					dut_p8_objattr_p->dut_p8_1objtype);
				STcopy(DUT_M26_INDEX,
					dut_p8_objattr_p->dut_p8_1objtype);
				break;
		}
		break;
    }
    switch (dut_p8_objattr_p->dut_p8_8system_use[0])
    {
	case 'S':
	    STcopy(DUT_M32_SYSTEM_OBJ,
			dut_p8_objattr_p->dut_p8_8system_use);
	    break;
	case 'U':
	    STcopy(DUT_M33_USER_OBJ,
			dut_p8_objattr_p->dut_p8_8system_use);
	    break;
    }

    if (!STcompare(dut_p8_objattr_p->dut_p8_6objsubtype, "N"))
    {
	STcopy(DUT_M34_NATIVE_OBJ,
		dut_p8_objattr_p->dut_p8_6objsubtype);
    }
    else if (!STcompare(dut_p8_objattr_p->dut_p8_6objsubtype, "L"))
    {
	STcopy(DUT_M35_REG_OBJ,
		dut_p8_objattr_p->dut_p8_6objsubtype);
    }
    
    EXEC FRS display dut_p8_objattr;

    EXEC FRS initialize;
    EXEC FRS begin;
	EXEC FRS putform dut_p8_objattr
	    (
	    dut_p8_1objtype	  = :dut_p8_objattr_p->dut_p8_1objtype,
	    dut_p8_2objname	  = :dut_p8_objattr_p->dut_p8_2objname,
	    dut_p8_3objowner	  = :dut_p8_objattr_p->dut_p8_3objowner,
	    dut_p8_4create_date	  = :dut_p8_objattr_p->dut_p8_4create_date,
	    dut_p8_5alter_date	  = :dut_p8_objattr_p->dut_p8_5alter_date,
	    dut_p8_6objsubtype	  = :dut_p8_objattr_p->dut_p8_6objsubtype,
	    dut_p8_7objversion	  = :dut_p8_objattr_p->dut_p8_7objversion,
	    dut_p8_8system_use	  = :dut_p8_objattr_p->dut_p8_8system_use,
	    dut_p8_9local_name    = :dut_p8_objattr_p->dut_p8_9local_name,
	    dut_p8_10dbmstype	  = :dut_p8_objattr_p->dut_p8_10dbmstype,
	    dut_p8_11nodename	  = :dut_p8_objattr_p->dut_p8_11nodename,
	    dut_p8_12ldbname	  = :dut_p8_objattr_p->dut_p8_12ldbname
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	EXEC FRS breakdisplay;
    EXEC FRS end;
}


/*
** Name: dut_pp9_1viewattr
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-jan-1989 (alexc)
**          Creation.
*/

DUT_STATUS
dut_pp9_1viewattr(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    DUT_T3_OBJLIST	*dut_t3_objlist_p;
    DUT_P9_VIEWATTR	*dut_p9_viewattr_p;
    DUT_P8_OBJATTR	*dut_p8_objattr_p;
    EXEC SQL end declare section;
    
    dut_t3_objlist_p	= dut_cb->dut_form_objlist->dut_f3_9objlist_curr_p;
    dut_p9_viewattr_p	= dut_cb->dut_popup_viewattr;
    dut_p8_objattr_p	= dut_cb->dut_popup_objattr;

    /* Get view information */

    if (dut_uu2_dbop(DUT_DIRECT_CONNECT_CDB, dut_cb, dut_errcb) 
	!= E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU1806_SV_CANT_DIRECT_CONNECT, 2,
		0, DUT_1CDB_LONG);
	return(DUT_ERROR);
    }

    EXEC SQL select
	table_type,
	table_subtype
    into
	:dut_cb->dut_c0_buffer,
	:dut_cb->dut_c0_buffer2
    from
	iidd_tables
    where
	iidd_tables.table_name = :dut_p8_objattr_p->dut_p8_2objname
		and
	iidd_tables.table_owner = :dut_p8_objattr_p->dut_p8_3objowner;

    STtrmwhite(dut_cb->dut_c0_buffer);
    STtrmwhite(dut_cb->dut_c0_buffer2);
    if (dut_cb->dut_c0_buffer2[0] == 'L')
    {
	/* "L" means Registered View, 
	** and "N" means native distributed View.
	*/
	dut_pp8_1objattr(dut_cb, dut_errcb);
	return(E_DU_OK);
    }
	
    EXEC SQL select
	table_name,
	table_owner,
	view_dml,
	check_option,
	text_sequence,
	text_segment
    into
	:dut_p9_viewattr_p->dut_p9_3view_name,
	:dut_p9_viewattr_p->dut_p9_4view_owner,
	:dut_p9_viewattr_p->dut_p9_5view_dml,
	:dut_p9_viewattr_p->dut_p9_6check_option,
	:dut_p9_viewattr_p->dut_p9_7text_sequence,
	:dut_p9_viewattr_p->dut_p9_8text_segment
    from
	iidd_views
    where
	(iidd_views.table_name = :dut_p9_viewattr_p->dut_p9_3view_name)
	    and
	(iidd_views.table_owner = :dut_p9_viewattr_p->dut_p9_4view_owner)
	    and
	(iidd_views.text_sequence = 1);

    EXEC FRS display dut_p9_viewattr;

    EXEC FRS initialize;
    EXEC FRS begin;
	EXEC FRS putform dut_p9_viewattr
	    (
	    dut_p9_3view_name	  = :dut_p9_viewattr_p->dut_p9_3view_name,
	    dut_p9_4view_owner	  = :dut_p9_viewattr_p->dut_p9_4view_owner,
	    dut_p9_5view_dml	  = :dut_p9_viewattr_p->dut_p9_5view_dml,
	    dut_p9_6check_option  = :dut_p9_viewattr_p->dut_p9_6check_option,
	    dut_p9_7text_sequence = :dut_p9_viewattr_p->dut_p9_7text_sequence,
	    dut_p9_8text_segment  = :dut_p9_viewattr_p->dut_p9_8text_segment
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	EXEC FRS breakdisplay;
    EXEC FRS end;
}


/*
** Name: dut_pp11_1perror	- print error
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      2-dec-1988 (alexc)
**          Creation.
*/

DUT_STATUS
dut_pp11_1perror(dut_msgstr)
EXEC SQL BEGIN DECLARE SECTION;
char	    dut_msgstr[];
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL whenever sqlerror call dut_ee5_none_starview;

    EXEC FRS display dut_p11_perror;

    EXEC FRS initialize;
    EXEC FRS begin;
    EXEC FRS putform dut_p11_perror
        (
	dut_p11_1msgstr	= :dut_msgstr
	);
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	EXEC FRS breakdisplay;
    EXEC FRS end;
    return(DUT_ERROR);
}


/*
** Name: dut_pp12_1ldblist
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      2-dec-1988 (alexc)
**          Creation.
*/
void
dut_pp12_1ldblist(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    i4		n_rows;
    EXEC SQL END DECLARE SECTION;
    DUT_P12_LDBLIST *dut_p12_ldblist_p;

    EXEC SQL whenever sqlerror call dut_ee5_none_starview;

    dut_p12_ldblist_p = dut_cb->dut_popup_ldblist;

    EXEC FRS inittable dut_p12_ldblist dut_p12_1ldblist read;
    EXEC FRS display dut_p12_ldblist;

    EXEC FRS initialize;
    EXEC FRS begin;
	dut_cb->dut_c0_buffer[0] = (char)EOS;
	dut_cb->dut_c8_status = E_DU_OK;
	dut_pp12_2load_ldblist(dut_cb, dut_errcb);
	if (dut_cb->dut_c8_status == DUT_ERROR)
	{
	    EXEC FRS breakdisplay;
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'Select', frskey4;
    EXEC FRS begin;
	EXEC FRS inquire_frs table dut_p12_ldblist
	    (
	    :n_rows = lastrow(dut_p12_1ldblist)
	    );
	if (n_rows == 0)
	{
	    dut_cb->dut_c0_buffer2[0] = EOS;
	    EXEC FRS breakdisplay;
	}
	EXEC FRS getrow dut_p12_ldblist dut_p12_1ldblist
	    (
	    :dut_cb->dut_c0_buffer = dut_t12_1ldbname
	    );
	EXEC FRS breakdisplay;
    EXEC FRS end;

    EXEC FRS activate menuitem 'Top', frskey5 ;
    EXEC FRS begin;
	dut_uu7_table_top(NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Bottom', frskey6 ;
    EXEC FRS begin;
	dut_uu8_table_bottom(NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Find', frskey7 ;
    EXEC FRS begin;
	dut_uu9_table_find(dut_cb, dut_errcb, DUT_M36_LDB_NAME, NULL, NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Help', frskey1;
    EXEC FRS begin;
	EXEC FRS HELP_FRS
	    (
	    subject	= 'Distributed Database Help',
	    file	= :dut_cb->dut_helpcb->dut_hp12_1name
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	EXEC FRS breakdisplay;
    EXEC FRS end;
}


/*
** Name: dut_pp12_2load_ldblist
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      2-dec-1988 (alexc)
**          Creation.
*/

DUT_STATUS
dut_pp12_2load_ldblist(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    DUT_P12_LDBLIST	*dut_p12_ldblist_p;
    DUT_T12_LDBLIST	*dut_t12_ldblist_p;
    DUT_T12_LDBLIST	*dut_t12_ldblist_curr_p;
    i4			dut_p12_0count;
    EXEC SQL END DECLARE SECTION;
    i4			dut_p12_0fill;

    EXEC SQL whenever sqlerror call dut_ee5_none_starview;

    dut_p12_0count	= 0;
    dut_p12_0fill	= 0;
    dut_p12_ldblist_p = (DUT_P12_LDBLIST *)dut_cb->dut_popup_ldblist;
    dut_t12_ldblist_p =
	(DUT_T12_LDBLIST *)dut_cb->dut_popup_ldblist->dut_p12_1ldblist;

    if (dut_uu2_dbop(DUT_DIRECT_CONNECT_CDB, dut_cb, dut_errcb) 
	!= E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU1806_SV_CANT_DIRECT_CONNECT, 2,
		0, DUT_1CDB_LONG);
	return(DUT_ERROR);
    }

    EXEC SQL select distinct
	count(*) into :dut_p12_0count
	from iidd_ddb_ldbids;

    if (dut_p12_0count > dut_p12_ldblist_p->dut_p12_0size)
    {
	MEfree(dut_p12_ldblist_p->dut_p12_1ldblist);
	dut_p12_ldblist_p->dut_p12_1ldblist = NULL;
	dut_t12_ldblist_p = NULL;
    }

    if (dut_t12_ldblist_p == NULL)
    {
	dut_t12_ldblist_p = (DUT_T12_LDBLIST *)MEreqmem(0,
		(dut_p12_0count + DUT_ME_EXTRA) * sizeof(DUT_T12_LDBLIST),
		TRUE, NULL);
	dut_p12_ldblist_p->dut_p12_1ldblist
		= (DUT_T12_LDBLIST *)dut_t12_ldblist_p;
	dut_p12_ldblist_p->dut_p12_0size = (dut_p12_0count + DUT_ME_EXTRA);
    }

    dut_t12_ldblist_curr_p = (DUT_T12_LDBLIST *)dut_t12_ldblist_p;

    if (dut_cb->dut_c9_status == DUT_P_POPUP_2)
    {
	EXEC FRS loadtable dut_p12_ldblist dut_p12_1ldblist
	    (
	    dut_t12_1ldbname = '*'
	    );
    }
	
    dut_cb->dut_on_error_exit = TRUE;

    EXEC SQL
	SELECT	distinct
	    trim(ldb_database)
	into
	    :dut_t12_ldblist_p->dut_t12_1ldbname
	from iidd_ddb_ldbids
	order by 1;
    EXEC SQL begin;
	EXEC FRS loadtable dut_p12_ldblist dut_p12_1ldblist
	    (
	    dut_t12_1ldbname = :dut_t12_ldblist_p->dut_t12_1ldbname
	    );
	
	dut_p12_0fill++;
	dut_t12_ldblist_p++;
	if (sqlca.sqlcode < 0)
	{
	    dut_cb->dut_c8_status = DUT_ERROR;
	    EXEC SQL endselect;
	}
    EXEC SQL end;

    dut_cb->dut_on_error_exit = FALSE;

    dut_p12_ldblist_p->dut_p12_0num_entry = dut_p12_0fill;

}


/*
** Name: dut_pp13_1ownlist
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      7-apr-1989 (alexc)
**          Creation.
*/
void
dut_pp13_1ownlist(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    i4		n_rows;
    EXEC SQL END DECLARE SECTION;
    DUT_P13_OWNLIST *dut_p13_ownlist_p;

    EXEC SQL whenever sqlerror call dut_ee5_none_starview;

    dut_p13_ownlist_p = dut_cb->dut_popup_ownlist;

    EXEC FRS inittable dut_p13_ownlist dut_p13_1ownlist read;
    EXEC FRS display dut_p13_ownlist;

    EXEC FRS initialize;
    EXEC FRS begin;
	dut_cb->dut_c0_buffer[0] = (char)EOS;
	dut_cb->dut_c8_status = E_DU_OK;
	dut_pp13_2load_ownlist(dut_cb, dut_errcb);
	if (dut_cb->dut_c8_status == DUT_ERROR)
	{
	    EXEC FRS breakdisplay;
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'Select', frskey4;
    EXEC FRS begin;
	EXEC FRS inquire_frs table dut_p13_ownlist
	    (
	    :n_rows = lastrow(dut_p13_1ownlist)
	    );
	if (n_rows == 0)
	{
	    dut_cb->dut_c0_buffer2[0] = EOS;
	    EXEC FRS breakdisplay;
	}
	EXEC FRS getrow dut_p13_ownlist dut_p13_1ownlist
	    (
	    :dut_cb->dut_c0_buffer = dut_t13_1owner
	    );
	EXEC FRS breakdisplay;
    EXEC FRS end;

    EXEC FRS activate menuitem 'Top', frskey5 ;
    EXEC FRS begin;
	dut_uu7_table_top(NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Bottom', frskey6 ;
    EXEC FRS begin;
	dut_uu8_table_bottom(NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Find', frskey7 ;
    EXEC FRS begin;
	dut_uu9_table_find(dut_cb, dut_errcb, DUT_M37_OWNER_NAME, NULL, NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Help', frskey1;
    EXEC FRS begin;
	EXEC FRS HELP_FRS
	    (
	    subject	= 'Distributed Database Help',
	    file	= :dut_cb->dut_helpcb->dut_hp13_1name
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	EXEC FRS breakdisplay;
    EXEC FRS end;
}


/*
** Name: dut_pp13_2load_ownlist
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      7-apr-1989 (alexc)
**          Creation.
*/

DUT_STATUS
dut_pp13_2load_ownlist(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    DUT_P13_OWNLIST	*dut_p13_ownlist_p;
    DUT_T13_OWNLIST	*dut_t13_ownlist_p;
    DUT_T13_OWNLIST	*dut_t13_ownlist_curr_p;
    i4			dut_p13_0count;
    EXEC SQL END DECLARE SECTION;
    i4			dut_p13_0fill;

    EXEC SQL whenever sqlerror call dut_ee5_none_starview;

    dut_p13_0count	= 0;
    dut_p13_0fill	= 0;
    dut_p13_ownlist_p = (DUT_P13_OWNLIST *)dut_cb->dut_popup_ownlist;
    dut_t13_ownlist_p =
	(DUT_T13_OWNLIST *)dut_cb->dut_popup_ownlist->dut_p13_1ownlist;

    if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) 
	!= E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU1805_SV_CANT_OPEN_DDB, 2, 0, 
			dut_cb->dut_c1_ddbname);
	return(DUT_ERROR);
    }

    EXEC SQL select distinct
	count(object_owner) into :dut_p13_0count
	from iiddb_objects;

    if (dut_p13_0count > dut_p13_ownlist_p->dut_p13_0size)
    {
	MEfree(dut_p13_ownlist_p->dut_p13_1ownlist);
	dut_p13_ownlist_p->dut_p13_1ownlist = NULL;
	dut_t13_ownlist_p = NULL;
    }

    if (dut_t13_ownlist_p == NULL)
    {
	dut_t13_ownlist_p = (DUT_T13_OWNLIST *)MEreqmem(0,
		(dut_p13_0count + DUT_ME_EXTRA) * sizeof(DUT_T13_OWNLIST),
		TRUE, NULL);
	dut_p13_ownlist_p->dut_p13_1ownlist
		= (DUT_T13_OWNLIST *)dut_t13_ownlist_p;
	dut_p13_ownlist_p->dut_p13_0size = (dut_p13_0count + DUT_ME_EXTRA);
    }

    dut_t13_ownlist_curr_p = (DUT_T13_OWNLIST *)dut_t13_ownlist_p;

    if (dut_cb->dut_c9_status == DUT_P_POPUP_2)
    {
	EXEC FRS loadtable dut_p13_ownlist dut_p13_1ownlist
	    (
	    dut_t13_1owner = '*'
	    );
    }
	
    dut_cb->dut_on_error_exit = TRUE;

    EXEC SQL
	SELECT	distinct
	    trim(object_owner)
	into
	    :dut_t13_ownlist_p->dut_t13_1owner
	from iiddb_objects
	order by 1;
    EXEC SQL begin;
	EXEC FRS loadtable dut_p13_ownlist dut_p13_1ownlist
	    (
	    dut_t13_1owner = :dut_t13_ownlist_p->dut_t13_1owner
	    );
	
	dut_p13_0fill++;
	dut_t13_ownlist_p++;
	if (sqlca.sqlcode < 0)
	{
	    dut_cb->dut_c8_status = DUT_ERROR;
	    EXEC SQL endselect;
	}
    EXEC SQL end;

    dut_cb->dut_on_error_exit = FALSE;

    dut_p13_ownlist_p->dut_p13_0num_entry = dut_p13_0fill;

}


/*
** Name: dut_pp14_1browse	- Browsing Nodes, databases, and tables.
**
** Description:
**      Browse the node name.
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      3-may-1989 (alexc)
**          Creation.
*/

DUT_STATUS
dut_pp14_1browse(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    /* Load GCA file information into array */
    /* Allow selection of Node name, or user entered node name */
    /* Call browse_ldb */
    if (dut_pp14_1browse_node(dut_cb, dut_errcb) == E_DU_OK)
    {
    	return(E_DU_OK);
    }
}

/*
** Name: dut_pp14_1browse_node	- Browsing Nodes, databases, and tables.
**
** Description:
**      Browse the node name.
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      3-may-1989 (alexc)
**          Creation.
*/

DUT_STATUS
dut_pp14_1browse_node(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    DUT_P14_NODE	*dut_p14_node_p;
    DUT_T14_NODE	*dut_t14_node_p;
    EXEC SQL end declare section;
    i4			dut_error_status;

    dut_p14_node_p = dut_cb->dut_popup_b_node;
    dut_t14_node_p = dut_p14_node_p->dut_p14_1node;

    EXEC FRS inittable dut_p14_node dut_p14_1node read;
    EXEC FRS display dut_p14_node;

    EXEC FRS initialize;
    EXEC FRS begin;
	dut_cb->dut_c8_status = E_DU_OK;
	dut_error_status = dut_uu24_gcf_node(dut_cb, dut_errcb);
	if (dut_error_status != DUT_ERROR &&
	    dut_cb->dut_c8_status != DUT_ERROR)
	{
	    dut_pp14_2load_node(dut_cb, dut_errcb);
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'Select', frskey4;
    EXEC FRS begin;
	EXEC FRS getrow dut_p14_node dut_p14_1node
	    (
	    :dut_cb->dut_c0_buffer = dut_t14_1node
	    );
	STcopy(dut_cb->dut_c0_buffer, dut_cb->dut_c16_browse_node);
	if  (dut_cb->dut_c0_buffer[0] != EOS)
	{
	    /* check for connectability first */
	    dut_pp15_1ldb(dut_cb, dut_errcb);
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'Top', frskey5 ;
    EXEC FRS begin;
	dut_uu7_table_top("dut_p14_1node");
    EXEC FRS end;

    EXEC FRS activate menuitem 'Bottom', frskey6 ;
    EXEC FRS begin;
	dut_uu8_table_bottom("dut_p14_1node");
    EXEC FRS end;

    EXEC FRS activate menuitem 'Find', frskey7 ;
    EXEC FRS begin;
	dut_uu9_table_find(dut_cb, dut_errcb,
		DUT_M08_NODENAME, "dut_p14_1node", NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Help', frskey1;
    EXEC FRS begin;
	EXEC FRS HELP_FRS
	    (
	    subject	= 'Distributed Database Help',
	    file	= :dut_cb->dut_helpcb->dut_hp14_1name
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	EXEC FRS breakdisplay;
    EXEC FRS end;
    return(E_DU_OK);
}

/*
** Name: dut_pp14_2load_node	- Load node into popup.
**
** Description:
**      Load nodes form dut_cb->dut_p14_node into the popup.
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-may-1989 (alexc)
**          Creation.
*/

DUT_STATUS
dut_pp14_2load_node(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    DUT_P14_NODE	*dut_p14_node_p;
    DUT_T14_NODE	*dut_t14_node_p;
    DUT_T14_NODE	*dut_t14_node_curr_p;
    EXEC SQL end declare section;
    i4			dut_p14_0fill;

    dut_p14_node_p = dut_cb->dut_popup_b_node;
    dut_t14_node_p = dut_p14_node_p->dut_p14_1node;
    dut_t14_node_curr_p = dut_t14_node_p;

    EXEC FRS clear field dut_p14_1node;
    for (dut_p14_0fill = 0;
	dut_p14_0fill < dut_p14_node_p->dut_p14_0num_entry;
	dut_p14_0fill++)
    {
	EXEC FRS loadtable dut_p14_node dut_p14_1node
	    (
	    dut_t14_1node = :dut_t14_node_curr_p->dut_t14_1node
	    );
	dut_t14_node_curr_p++;
    }
    EXEC FRS redisplay;
    return(E_DU_OK);
}

/*
** Name: dut_pp15_1ldb	- Browse Local Database Names.
**
** Description:
**
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-may-1989 (alexc)
**          Creation.
*/

DUT_STATUS
dut_pp15_1ldb(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    DUT_P15_LDB	*dut_p15_ldb_p;
    DUT_T15_LDB	*dut_t15_ldb_p;
    DUT_T15_LDB	*dut_t15_ldb_curr_p;
    i4		dut_p15_0count;
    EXEC SQL end declare section;
    i4		dut_p15_0fill;

    dut_p15_ldb_p = dut_cb->dut_popup_b_ldb;
    dut_t15_ldb_p = dut_p15_ldb_p->dut_p15_1ldb;
    dut_t15_ldb_curr_p = dut_t15_ldb_p;

    STcopy("iidbdb", dut_cb->dut_c3_ldbname);
    STcopy(dut_cb->dut_c16_browse_node, dut_cb->dut_c4_ldbnode);

    STprintf(dut_cb->dut_c0_buffer, "%s::%s",
			dut_cb->dut_c4_ldbnode,
			"iidbdb");

    if (dut_uu2_dbop(DUT_DIRECT_CONNECT, dut_cb, dut_errcb) != E_DU_OK)
    {
        dut_ee1_error(dut_errcb, W_DU1806_SV_CANT_DIRECT_CONNECT,
			2, 0, dut_cb->dut_c0_buffer);
	return(DUT_ERROR);
    }

    EXEC SQL select count(*)
	into :dut_p15_0count
	from iidatabase;

    dut_cb->dut_on_error_exit = FALSE;

    if (dut_p15_0count > dut_p15_ldb_p->dut_p15_0size)
    {
	MEfree(dut_t15_ldb_p);
	dut_p15_ldb_p->dut_p15_1ldb = NULL;
	dut_t15_ldb_p = NULL;
    }

    if (dut_t15_ldb_p == NULL)
    {
	dut_t15_ldb_p = (DUT_T15_LDB *)MEreqmem(0,
		(dut_p15_0count + DUT_ME_EXTRA) * sizeof(DUT_T15_LDB),
		TRUE, NULL);
	dut_p15_ldb_p->dut_p15_1ldb = dut_t15_ldb_p;
	dut_cb->dut_popup_b_ldb->dut_p15_0size
		= dut_p15_0count + DUT_ME_EXTRA;
    }

    dut_t15_ldb_curr_p = dut_t15_ldb_p;

    EXEC FRS inittable dut_p15_ldb dut_p15_1ldb read;
    EXEC FRS display dut_p15_ldb;

    EXEC FRS initialize;
    EXEC FRS begin;
	dut_cb->dut_on_error_exit = TRUE;

	EXEC SQL select
	    trim(name)
	into
	    :dut_t15_ldb_curr_p->dut_t15_1ldb
	from 
	    iidatabase
	where
	    not dbservice = 1
	order by 1;
	EXEC SQL begin;
	    STcopy("ingres", dut_t15_ldb_curr_p->dut_t15_2dbms_type);
	    EXEC FRS loadtable dut_p15_ldb dut_p15_1ldb
		(
		dut_t15_1ldb	= :dut_t15_ldb_curr_p->dut_t15_1ldb,
		dut_t15_2dbms_type = :dut_t15_ldb_curr_p->dut_t15_2dbms_type
		);
	    dut_p15_0fill++;
	    dut_t15_ldb_curr_p++;
	    if (sqlca.sqlcode < 0)
	    {
	    	dut_cb->dut_c8_status = DUT_ERROR;
	    	EXEC SQL endselect;
	    }
	EXEC SQL end;

	dut_cb->dut_on_error_exit = FALSE;

	dut_p15_ldb_p->dut_p15_0num_entry = dut_p15_0fill;
    EXEC FRS end;

    EXEC FRS activate menuitem 'Select', frskey4;
    EXEC FRS begin;
	EXEC FRS getrow dut_p15_ldb dut_p15_1ldb
	    (
	    :dut_cb->dut_c0_buffer = dut_t15_1ldb,
	    :dut_cb->dut_c0_buffer3 = dut_t15_2dbms_type
	    );
	STcopy(dut_cb->dut_c0_buffer, dut_cb->dut_c17_browse_database);
	STcopy(dut_cb->dut_c0_buffer3, dut_cb->dut_c18_browse_dbms);
	dut_cb->dut_on_error_exit = FALSE;
	dut_pp16_1browse_table(dut_cb, dut_errcb);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Top', frskey5 ;
    EXEC FRS begin;
	dut_uu7_table_top(NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Bottom', frskey6 ;
    EXEC FRS begin;
	dut_uu8_table_bottom(NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Find', frskey7 ;
    EXEC FRS begin;
	dut_uu9_table_find(dut_cb, dut_errcb,
		DUT_M36_LDB_NAME, "dut_p15_1ldb", NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Help', frskey1;
    EXEC FRS begin;
	EXEC FRS HELP_FRS
	    (
	    subject	= 'Distributed Database Help',
	    file	= :dut_cb->dut_helpcb->dut_hp15_1name
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	EXEC FRS breakdisplay;
    EXEC FRS end;

    return(E_DU_OK);
}


/*
** Name: dut_pp16_1browse_table	- Browse Local Database Names.
**
** Description:
**
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-may-1989 (alexc)
**          Creation.
**	06-jun-1991 (fpang)
**	    If dut_pp16_3browse_load_table() fails, ldb cannot be reached,
**	    so go back to previous form.
*/

void
dut_pp16_1browse_table(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    DUT_P16_TABLE	*dut_p16_table_p;
    DUT_T16_TABLE	*dut_t16_table_p;
    DUT_T16_TABLE	*dut_t16_table_curr_p;
    i4		dut_p16_0count;
    char	dut_dbname[DDB_2MAXNAME];
    EXEC SQL end declare section;
    i4		dut_p16_0fill;

    dut_p16_table_p = dut_cb->dut_popup_b_table;
    dut_t16_table_p = dut_p16_table_p->dut_p16_1table;
    dut_t16_table_curr_p = dut_t16_table_p;

    STcopy(dut_cb->dut_c16_browse_node, dut_cb->dut_c4_ldbnode);
    STcopy(dut_cb->dut_c17_browse_database, dut_cb->dut_c3_ldbname);

    STprintf(dut_cb->dut_c0_buffer, "%s::%s",
		dut_cb->dut_c4_ldbnode,
		dut_cb->dut_c3_ldbname);

    EXEC FRS inittable dut_p16_table dut_p16_1table read;
    EXEC FRS display dut_p16_table;

    EXEC FRS initialize;
    EXEC FRS begin;
	if (dut_pp16_3browse_load_table(dut_cb, dut_errcb) != E_DU_OK)
	    EXEC FRS breakdisplay;
    EXEC FRS end;

    EXEC FRS activate menuitem 'Register', frskey4;
    EXEC FRS begin;
        if (dut_p16_table_p->dut_p16_0num_entry == 0)
        {
		dut_ee1_error(dut_errcb, W_DU181D_SV_EMPTY_LDB, 0);
		EXEC FRS redisplay;
	} 
	else
	{
	    EXEC FRS getrow dut_p16_table dut_p16_1table
	        (
	        :dut_cb->dut_c0_buffer = dut_t16_1table,
	        :dut_cb->dut_c0_buffer2 = dut_t16_2own
	        );
	    STcopy(dut_cb->dut_c0_buffer, dut_cb->dut_c19_browse_table);
	    STcopy(dut_cb->dut_c0_buffer2, dut_cb->dut_c20_browse_own);
	    dut_pp16_2register(dut_cb, dut_errcb);

	    /* Restore connection to browse ldb */
    	    STcopy(dut_cb->dut_c16_browse_node, dut_cb->dut_c4_ldbnode);
    	    STcopy(dut_cb->dut_c17_browse_database, dut_cb->dut_c3_ldbname);

    	    STprintf(dut_cb->dut_c0_buffer, "%s::%s",
	   	     dut_cb->dut_c4_ldbnode,
		     dut_cb->dut_c3_ldbname);

    	    if (dut_uu2_dbop(DUT_DIRECT_CONNECT, dut_cb, dut_errcb) != E_DU_OK)
    	    {
                dut_ee1_error(dut_errcb, W_DU1806_SV_CANT_DIRECT_CONNECT,
			2, 0, dut_cb->dut_c0_buffer);
    	    }
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'SQL';
    EXEC FRS begin;
	STcopy(dut_cb->dut_c16_browse_node, dut_cb->dut_c4_ldbnode);
	STcopy(dut_cb->dut_c17_browse_database, dut_cb->dut_c3_ldbname);

	STprintf(dut_cb->dut_c0_buffer, "%s::%s",
		dut_cb->dut_c4_ldbnode,
		dut_cb->dut_c3_ldbname);

	if (dut_uu2_dbop(DUT_DIRECT_CONNECT, dut_cb, dut_errcb) != E_DU_OK)
	{
            dut_ee1_error(dut_errcb, W_DU1806_SV_CANT_DIRECT_CONNECT,
			2, 0, dut_cb->dut_c0_buffer);
	    EXEC FRS breakdisplay;
	}

	if ((dut_cb->dut_c16_browse_node[0] == EOS) ||
	    !STcompare(dut_cb->dut_c1_ddbname, dut_cb->dut_c13_vnodename))
	{
	    STprintf(dut_dbname, "%s",
			dut_cb->dut_c17_browse_database);
	}
	else
	{
	    STprintf(dut_dbname, "%s::%s",
			dut_cb->dut_c2_nodename,
			dut_cb->dut_c17_browse_database);
	}
	STprintf(dut_cb->dut_c0_buffer, "ISQL %s", dut_dbname);
	EXEC FRS message :dut_cb->dut_c0_buffer;
	EXEC SQL commit;
	EXEC SQL call isql (database = :dut_dbname);
	dut_cb->dut_on_error_noop = TRUE;
	EXEC SQL commit;
	EXEC SQL select dbmsinfo('database') into :dut_cb->dut_c0_buffer;
	/* Want to set it to direct disconnect mode */
	STtrmwhite(dut_cb->dut_c0_buffer);
	if (!STcompare(dut_cb->dut_c0_buffer, dut_cb->dut_c1_ddbname))
	{
	    /* Already direct disconnected in the sub-call */
	}
	else
	{
	    EXEC SQL direct disconnect;
	}
	dut_cb->dut_c7_status = DUT_S_CONNECT_DDB;
	EXEC SQL commit;
	dut_cb->dut_on_error_noop = FALSE;
	dut_pp16_3browse_load_table(dut_cb, dut_errcb);
	EXEC FRS redisplay;
    EXEC FRS end;

    EXEC FRS activate menuitem 'Tables';
    EXEC FRS begin;
	STcopy(dut_cb->dut_c16_browse_node, dut_cb->dut_c4_ldbnode);
	STcopy(dut_cb->dut_c17_browse_database, dut_cb->dut_c3_ldbname);

	STprintf(dut_cb->dut_c0_buffer, "%s::%s",
		dut_cb->dut_c4_ldbnode,
		dut_cb->dut_c3_ldbname);

	if (dut_uu2_dbop(DUT_DIRECT_CONNECT, dut_cb, dut_errcb) != E_DU_OK)
	{
            dut_ee1_error(dut_errcb, W_DU1806_SV_CANT_DIRECT_CONNECT,
			2, 0, dut_cb->dut_c0_buffer);
	    EXEC FRS breakdisplay;
	}

	if ((dut_cb->dut_c2_nodename[0] == EOS) ||
	    !STcompare(dut_cb->dut_c2_nodename, dut_cb->dut_c13_vnodename))
	{
	    STprintf(dut_dbname, "%s",
			dut_cb->dut_c17_browse_database);
	}
	else
	{
	    STprintf(dut_dbname, "%s::%s",
			dut_cb->dut_c2_nodename,
			dut_cb->dut_c17_browse_database);
	}
	STprintf(dut_cb->frs_message, "TABLES %s", dut_dbname);
	EXEC FRS message :dut_cb->frs_message;
	EXEC SQL commit;
	EXEC SQL call tables (database = :dut_dbname);
	dut_cb->dut_on_error_noop = TRUE;
	EXEC SQL commit;
	EXEC SQL select dbmsinfo('database') into :dut_cb->dut_c0_buffer;
	/* Want to set it to direct disconnect mode */
	STtrmwhite(dut_cb->dut_c0_buffer);
	if (!STcompare(dut_cb->dut_c0_buffer, dut_cb->dut_c1_ddbname))
	{
	    /* Already direct disconnected in the sub-call */
	}
	else
	{
	    EXEC SQL direct disconnect;
	}
	dut_cb->dut_c7_status = DUT_S_CONNECT_DDB;
	EXEC SQL commit;
	dut_cb->dut_on_error_noop = FALSE;
	dut_pp16_3browse_load_table(dut_cb, dut_errcb);
	EXEC FRS redisplay;
    EXEC FRS end;

    EXEC FRS activate menuitem 'Top', frskey5 ;
    EXEC FRS begin;
	dut_uu7_table_top(NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Bottom', frskey6 ;
    EXEC FRS begin;
	dut_uu8_table_bottom(NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Find', frskey7 ;
    EXEC FRS begin;
	dut_uu9_table_find(dut_cb, dut_errcb, 
		DUT_M04_OBJNAME, "dut_p16_1table", NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Help', frskey1;
    EXEC FRS begin;
	EXEC FRS HELP_FRS
	    (
	    subject	= 'Distributed Database Help',
	    file	= :dut_cb->dut_helpcb->dut_hp16_1name
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	EXEC FRS breakdisplay;
    EXEC FRS end;

}


/*
** Name: dut_pp16_2register	- Register the table object.
**
** Description:
**
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-may-1989 (alexc)
**          Creation.
*/

DUT_STATUS
dut_pp16_2register(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    char		*register_call;
    char		msg_buffer[DB_SHORTMSG];
    char		dut_p16_reg_buffer[DB_LONGMSG];
    DUT_P3_REGISTER	*dut_p16_register_p;
    DUT_F3_OBJLIST	*dut_f16_objlist_p;
    EXEC SQL END DECLARE SECTION;
    i4			bad_param;

    dut_cb->dut_on_error_exit = FALSE;

    dut_f16_objlist_p = dut_cb->dut_form_objlist;

    dut_p16_register_p = dut_cb->dut_popup_register;

    if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) 
	!= E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU1805_SV_CANT_OPEN_DDB, 2, 0, 
			dut_cb->dut_c1_ddbname);
	return(DUT_ERROR);
    }

    EXEC FRS display dut_p3_register;

    EXEC FRS initialize;
    EXEC FRS begin;
	STcopy(dut_cb->dut_c19_browse_table,
			dut_p16_register_p->dut_p3_1table);
	STcopy(dut_cb->dut_c19_browse_table,
			dut_p16_register_p->dut_p3_2source);
	STcopy(dut_cb->dut_c18_browse_dbms, 
			dut_p16_register_p->dut_p3_3dbms);
	STcopy(dut_cb->dut_c16_browse_node,
			dut_p16_register_p->dut_p3_4node);
	STcopy(dut_cb->dut_c17_browse_database,
			dut_p16_register_p->dut_p3_5database);
	EXEC FRS putform dut_p3_register
	    (
	    dut_p3_1table	= :dut_p16_register_p->dut_p3_1table,
	    dut_p3_2source	= :dut_p16_register_p->dut_p3_2source,
	    dut_p3_3dbms	= :dut_p16_register_p->dut_p3_3dbms,
	    dut_p3_4node	= :dut_p16_register_p->dut_p3_4node,
	    dut_p3_5database	= :dut_p16_register_p->dut_p3_5database
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'Create', frskey4;
    EXEC FRS begin;
	bad_param = TRUE;
	while (bad_param)
	{
	    bad_param = FALSE;

	    EXEC FRS getform dut_p3_register
	    	(
	    	:dut_p16_register_p->dut_p3_1table:dut_cb->dut_null_1
							= dut_p3_1table,
	    	:dut_p16_register_p->dut_p3_2source:dut_cb->dut_null_2
							= dut_p3_2source,
	    	:dut_p16_register_p->dut_p3_3dbms:dut_cb->dut_null_3
							= dut_p3_3dbms,
	    	:dut_p16_register_p->dut_p3_4node:dut_cb->dut_null_4
							= dut_p3_4node,
	    	:dut_p16_register_p->dut_p3_5database:dut_cb->dut_null_5
							= dut_p3_5database
	    	);
	    if (dut_cb->dut_null_1 == -1)
	    {
	    	dut_p16_register_p->dut_p3_1table[0] = EOS;
		bad_param = TRUE;
	    }
	    if (dut_cb->dut_null_2 == -1)
	    {
	    	dut_p16_register_p->dut_p3_2source[0] = EOS;
		bad_param = TRUE;
	    }
	    if (dut_cb->dut_null_3 == -1)
	    {
	    	dut_p16_register_p->dut_p3_3dbms[0] = EOS;
		bad_param = TRUE;
	    }
	    if (dut_cb->dut_null_4 == -1)
	    {
	    	dut_p16_register_p->dut_p3_4node[0] = EOS;
		bad_param = TRUE;
	    }
	    if (dut_cb->dut_null_5 == -1)
	    {
	    	dut_p16_register_p->dut_p3_5database[0] = EOS;
		bad_param = TRUE;
	    }
	    if (dut_p16_register_p->dut_p3_1table[0] == 'i' 
		&& dut_p16_register_p->dut_p3_1table[1] == 'i')
	    {
		bad_param = TRUE;
	    }
	    if (bad_param == TRUE)
	    {
		dut_ee1_error(dut_errcb, W_DU181A_SV_CANT_REG_FROM_CRIT, 0);
	    }
	}
	CVlower(dut_p16_register_p->dut_p3_1table);
	STtrmwhite(dut_p16_register_p->dut_p3_1table);
	CVlower(dut_p16_register_p->dut_p3_2source);
	STtrmwhite(dut_p16_register_p->dut_p3_2source);
	STtrmwhite(dut_p16_register_p->dut_p3_3dbms);
	CVlower(dut_p16_register_p->dut_p3_4node);
	STtrmwhite(dut_p16_register_p->dut_p3_4node);
	CVlower(dut_p16_register_p->dut_p3_5database);
	STtrmwhite(dut_p16_register_p->dut_p3_5database);

/*
**	may need this condition later.
**	    dut_p16_register_p->dut_p3_3dbms[0]		!= EOS &&
*/
	if (bad_param == FALSE)
	{
	    /* If all data setup is okay, then go ahead and create
	    ** registration.
	    */
	    dut_cb->dut_on_error_exit = FALSE;
	    dut_cb->dut_on_error_noop = FALSE;
	    if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
	    {
		dut_ee1_error(dut_errcb, W_DU1805_SV_CANT_OPEN_DDB, 2, 0, 
				dut_cb->dut_c1_ddbname);
		EXEC FRS breakdisplay;
	    }
	    /* Set up registration string */
	    STprintf(dut_p16_reg_buffer,
		"register %s as link from %s \
		with dbms = %s, node = %s, database = %s",
		dut_p16_register_p->dut_p3_1table,
		dut_p16_register_p->dut_p3_2source,
		dut_p16_register_p->dut_p3_3dbms,
		dut_p16_register_p->dut_p3_4node,
		dut_p16_register_p->dut_p3_5database);
	    STprintf(msg_buffer,
			DUT_M40_CREATE_REGIST,
			dut_p16_register_p->dut_p3_1table);   
	    /* Go ahead and execute the registration string */
	    EXEC FRS message :msg_buffer;
	    EXEC SQL commit;
	    EXEC SQL execute immediate :dut_p16_reg_buffer;

	    STcopy(dut_p16_register_p->dut_p3_1table,
			dut_cb->dut_c0_buffer);
	    STcopy(dut_p16_register_p->dut_p3_0objowner,
			dut_cb->dut_c0_buffer2);

 	    if (sqlca.sqlcode < 0)
	    {
		dut_p16_register_p->dut_p3_4node[0] = EOS;
		dut_p16_register_p->dut_p3_5database[0] = EOS;
		dut_cb->dut_c8_status = DUT_NO_OBJLIST_CHANGE;
		dut_ee1_error(dut_errcb, W_DU1816_SV_CANT_REGISTER_TABLE,
			2, 0, dut_p16_register_p->dut_p3_1table);
	    }
	    else
	    {
		EXEC SQL begin declare section;
		DUT_T3_OBJLIST	*dut_t16_append_p;
		EXEC SQL end declare section;

		dut_cb->dut_c8_status = DUT_OBJLIST_CHANGED;
		if ((dut_cb->dut_form_objlist->dut_f3_0obj_num_entry+1) >
			dut_cb->dut_form_objlist->dut_f3_0obj_size)
		{
		    DUT_T3_OBJLIST	*tmp_t16_p;
		    DUT_T3_OBJLIST	*tmp_t16_1p;
		    DUT_T3_OBJLIST	*tmp_t16_2p;
		    i4			tmp_index;

		    /* Alloc new block */
		    dut_cb->dut_form_objlist->dut_f3_0obj_size += DUT_ME_EXTRA;
		    tmp_t16_p = (DUT_T3_OBJLIST *)MEreqmem(0,
				(dut_f16_objlist_p->dut_f3_0obj_size *
					sizeof(DUT_T3_OBJLIST)),
				TRUE, NULL);
		    /* Copy data form old to new */
		    for (tmp_index = 0, 
			tmp_t16_1p = dut_f16_objlist_p->dut_f3_9objlist,
			tmp_t16_2p = tmp_t16_p;
			tmp_index < dut_f16_objlist_p->dut_f3_0obj_num_entry;
			tmp_index++, tmp_t16_1p++, tmp_t16_2p++)
		    {
		    	MEcopy(tmp_t16_1p,
				sizeof(DUT_T3_OBJLIST),
				tmp_t16_2p);
		    }
		    /* Destroy old block */
		    MEfree(dut_cb->dut_form_objlist->dut_f3_9objlist);
		    dut_cb->dut_form_objlist->dut_f3_9objlist = tmp_t16_p;
		}
		dut_t16_append_p = (DUT_T3_OBJLIST *)
			(dut_cb->dut_form_objlist->dut_f3_9objlist +
			dut_cb->dut_form_objlist->dut_f3_0obj_num_entry);
		EXEC SQL commit;

		if (dut_uu2_dbop(DUT_DIRECT_CONNECT_CDB, dut_cb, dut_errcb) 
		    != E_DU_OK)
		{
		    dut_ee1_error(dut_errcb, W_DU1806_SV_CANT_DIRECT_CONNECT,
			2, 0, DUT_1CDB_LONG);
		    EXEC FRS breakdisplay;
		    return(DUT_ERROR);
		}

		/* Confirm registration */
		dut_cb->dut_on_error_exit = FALSE;

		EXEC SQL commit;
		EXEC SQL 
		    SELECT distinct
			iidd_ddb_objects.object_base,
			lowercase(trim(iidd_ddb_objects.object_name)),
			lowercase(trim(iidd_ddb_objects.object_owner)),
			lowercase(trim(iidd_tables.table_type)),
			lowercase(trim(iidd_tables.table_subtype)),
			lowercase(trim(iidd_ddb_objects.system_object)),
			lowercase(trim(iidd_ddb_ldbids.ldb_database)),
			lowercase(trim(iidd_ddb_ldbids.ldb_node))
		    into
			:dut_t16_append_p->dut_t3_0objbase,
			:dut_t16_append_p->dut_t3_1objname,
			:dut_t16_append_p->dut_t3_0objowner,
			:dut_t16_append_p->dut_t3_0objtype,
			:dut_t16_append_p->dut_t3_0subtype,
			:dut_t16_append_p->dut_t3_0system,
			:dut_t16_append_p->dut_t3_0ldbname,
			:dut_t16_append_p->dut_t3_0ldbnode
		    from
		        iidd_ddb_objects, iidd_ddb_tableinfo,
			iidd_ddb_ldbids, iidd_tables
		    where 
		        (iidd_ddb_objects.object_name
				= :dut_cb->dut_c0_buffer)
			and
			(iidd_tables.table_name
				= :dut_cb->dut_c0_buffer)
			and
			(iidd_ddb_objects.object_owner
				= :dut_cb->dut_c15_username)
			and
			(iidd_tables.table_owner
				= :dut_cb->dut_c15_username)
			and
			(iidd_ddb_objects.object_base
				= iidd_ddb_tableinfo.object_base)
			and
			(iidd_ddb_objects.object_index
				= iidd_ddb_tableinfo.object_index)
			and
			(iidd_ddb_ldbids.ldb_id
				= iidd_ddb_tableinfo.ldb_id);

		dut_cb->dut_on_error_exit = FALSE;
		CVupper(dut_t16_append_p->dut_t3_0system);
		CVupper(dut_t16_append_p->dut_t3_0objtype);
		dut_t16_append_p->dut_t3_0delete = FALSE;
		dut_cb->dut_form_objlist->dut_f3_0obj_num_entry++;

		MEcopy(dut_t16_append_p,
			sizeof(DUT_T3_OBJLIST),
			dut_cb->dut_form_objlist->dut_f3_9objlist_curr_p);

		dut_uu11_chk_criteria(dut_cb, dut_errcb, dut_t16_append_p);

		dut_uu13_buble_sort(dut_cb->dut_form_objlist->dut_f3_9objlist,
			sizeof(DUT_T3_OBJLIST), DDB_MAXNAME, 
			dut_cb->dut_form_objlist->dut_f3_0obj_num_entry);
	    }
	    EXEC FRS breakdisplay;
	}
	else
	{
	    /* Incomplete information for creating a Register */
	    dut_p16_register_p->dut_p3_1table[0] = EOS;
	    dut_ee1_error(dut_errcb, W_DU181A_SV_CANT_REG_FROM_CRIT, 0);
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'Help', frskey1;
    EXEC FRS begin;
	EXEC FRS HELP_FRS
	    (
	    subject	= 'Distributed Database Help',
	    file	= :dut_cb->dut_helpcb->dut_hp16_2name
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	EXEC FRS breakdisplay;
    EXEC FRS end;

    return(E_DU_OK);
}


/*
** Name: dut_pp16_3browse_load_table	- load table information with data
**
** Description:
**	clear table field and load it with local table names.
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-Aug-1989 (alexc)
**          Creation.
**	25-jul-1991 (fpang)
**	    Return E_DU_OK successful.
**
*/

DUT_STATUS
dut_pp16_3browse_load_table(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    DUT_P16_TABLE	*dut_p16_table_p;
    DUT_T16_TABLE	*dut_t16_table_p;
    DUT_T16_TABLE	*dut_t16_table_curr_p;
    i4		dut_p16_0count;
    char	dut_dbname[DDB_2MAXNAME];
    EXEC SQL end declare section;
    i4		dut_p16_0fill = 0;

    dut_cb->dut_on_error_noop = FALSE;
    dut_cb->dut_on_error_exit = FALSE; /* This could be a STAR or LOCAL
					* failure, that's why no exit.
					*/

    dut_p16_table_p = dut_cb->dut_popup_b_table;
    dut_t16_table_p = dut_p16_table_p->dut_p16_1table;
    dut_t16_table_curr_p = dut_t16_table_p;

    if (dut_uu2_dbop(DUT_DIRECT_CONNECT, dut_cb, dut_errcb) != E_DU_OK)
    {
        dut_ee1_error(dut_errcb, W_DU1806_SV_CANT_DIRECT_CONNECT,
			2, 0, dut_cb->dut_c0_buffer);
	return(DUT_ERROR);
    }

    dut_cb->dut_on_error_exit = TRUE;
    EXEC SQL select count(*)
	into 
	    :dut_p16_0count
	from
	    iitables;

    if (dut_p16_0count > dut_p16_table_p->dut_p16_0size)
    {
	MEfree(dut_t16_table_p);
	dut_p16_table_p->dut_p16_1table = NULL;
	dut_t16_table_p = NULL;
    }

    if (dut_t16_table_p == NULL)
    {
	dut_t16_table_p = (DUT_T16_TABLE *)MEreqmem(0,
		(dut_p16_0count + DUT_ME_EXTRA) * sizeof(DUT_T16_TABLE),
		TRUE, NULL);
	dut_p16_table_p->dut_p16_1table = dut_t16_table_p;
	dut_cb->dut_popup_b_table->dut_p16_0size
		= dut_p16_0count + DUT_ME_EXTRA;
    }

    EXEC FRS clear field dut_p16_1table;

    dut_t16_table_curr_p = dut_t16_table_p;

    EXEC SQL select
	trim(table_name),
	trim(table_owner),
	trim(table_type),
	trim(system_use)
    into
	:dut_t16_table_curr_p->dut_t16_1table,
	:dut_t16_table_curr_p->dut_t16_2own,
	:dut_t16_table_curr_p->dut_t16_3type,
	:dut_t16_table_curr_p->dut_t16_4system
    from 
	iitables
    order by 1;
    EXEC SQL begin;
	switch (dut_t16_table_curr_p->dut_t16_3type[0])
	{
	    case 'I':
		STcopy(DUT_M26_INDEX, dut_t16_table_curr_p->dut_t16_3type);
		break;
	    case 'V':
		STcopy(DUT_M24_VIEW, dut_t16_table_curr_p->dut_t16_3type);
		break;
	    case 'T':
		STcopy(DUT_M22_TABLE, dut_t16_table_curr_p->dut_t16_3type);
		break;
	}
	switch (dut_t16_table_curr_p->dut_t16_4system[0])
	{
	    case 'S':
		STcopy(DUT_M38_SYSTEM,
			dut_t16_table_curr_p->dut_t16_4system);
		break;
	    case 'U':
		STcopy(DUT_M39_USER,
			dut_t16_table_curr_p->dut_t16_4system);
		break;
	}

	/* Do not list objects that are system or Index */
	if (STcompare(dut_t16_table_curr_p->dut_t16_4system,
			DUT_M38_SYSTEM) &&
	    STcompare(dut_t16_table_curr_p->dut_t16_3type,
			DUT_M26_INDEX))
	{
	    EXEC FRS loadtable dut_p16_table dut_p16_1table
		(
		dut_t16_1table
			= :dut_t16_table_curr_p->dut_t16_1table,
		dut_t16_2own
			= :dut_t16_table_curr_p->dut_t16_2own,
		dut_t16_3type	
			= :dut_t16_table_curr_p->dut_t16_3type,
		dut_t16_4system
			= :dut_t16_table_curr_p->dut_t16_4system
		);
	    dut_p16_0fill++;
	    dut_t16_table_curr_p++;
	}
	if (sqlca.sqlcode < 0)
	{
	    dut_cb->dut_c8_status = DUT_ERROR;
	    EXEC SQL endselect;
	}
    EXEC SQL end;
    dut_cb->dut_on_error_exit = FALSE;
    dut_p16_table_p->dut_p16_0num_entry = dut_p16_0fill;
    return (E_DU_OK);
}
