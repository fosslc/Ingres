/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <dbms.h>
#include    <er.h>
#include    <duerr.h>
#include    <st.h>
#include    <nm.h>
#include    <me.h>
#include    <ut.h>
#include    <cv.h>
#include    <pc.h>
#include    <cv.h>

#include    "duustrings.h"

EXEC SQL INCLUDE 'dut.sh';
EXEC SQL INCLUDE SQLCA;

/**
**
**  Name: DUTFORM.SC - form utility functions.
**
**  Description:
**	This file contain functions that deals with the 3 full screen
**	forms used in starview.
**
**	dut_ff1_1intro		- Form 1, the introductory form.
**	dut_ff1_2load_ddb	- Load DDB names to form 1.
**	dut_ff2_1ldblist	- Form 2, the LDB list form.
**	dut_ff2_2load_ldb	- Load LDB info to form 2.
**	dut_ff2_3test_node	- TestNode submenu.
**	dut_ff2_4reload_ldb	- ReLoad LDB list info to form 2.
**	dut_ff2_5test_ldb	- TestLDB submenu.
**	dut_ff3_1objlist	- Form 3, the OBJECT list form.
**	dut_ff3_2load_obj	- Load Object info to form 3.
**	dut_ff3_3reload_obj	- Reload Object info to form 3.
**	dut_ff3_4objinfo	- Object information.
**	dut_ff3_5drop_object	- ***Removed, not use anymore.***
**	dut_ff3_6remove_object	- Remove a registered table.
**	dut_ff3_7objects	- Load objects from ddb.
**	dut_ff3_8fixlist        - Fix objects list after a delete.
**
**  History:
**      02-nov-1988 (alexc)
**          Creation.
**	08-mar-1989 (alexc)
**	    Alpha test version.
**      12-jun-1989 (alexc)
**          Modify remove function to take care of all register tables, and
**		register views.
**	06-jun-1991 (fpang)
**	    Changed copyright to 1991.
**	02-jul-1991 (rudyw)
**	    Create a line of forward function references to remedy a compiler
**	    complaint of redefinition.
**	09-jul-1991 (rudyw)
**	    Back out previous change which VMS compiler didn't accept.
**	10-jul-1991 (rudyw)
**	    Redo forward reference fix so acceptable to all boxes.
**	25-jul-1991 (fpang)
**	    1. dut_ff1_1intro()
**	       - Add activations to update ddbname and ddbnode fields when
**	         user moves up and down in the DdbHelp table field.
**	       - Removed the 'Select' menuitem.
**	       - Fixed Top/Bottom/Find to keep ddbname and ddbnode fields
**	         current when scrolling DdbHelp table field.
**	    2. dut_ff1_2load_ddb()
**	       - Explicitly return E_DU_OK if successful.
**	    3. dut_ff2_3test_node() and dut_ff2_5test_ldb()
**	       - Cleanup associations after AllLdbs and AllNodes sub menu
**	         selections only. Don't do it when 'End' is selected
**	         because connections were never made.
**	       - Nicer text when displaying node/ldb accessibility message.
**	    4. dut_ff3_1objlist()
**	       - Fix B38733, indexes still displayed when registered object
**	         with indexes is removed. The reverse was also true, when
**		 a local object with indexes was registered, indexes were not
**		 displayed.
**	       - Changed Browse/Remove to reload the objlist because they may
**		 may have changed as a result of the Register/Remove.
**	         Added dut_ff3_7objects() and dut_ff3_8fixlist() to support
**		 reloading of objlist.
**	12-sep-1991 (fpang)
**          1. Changed Star*View to StarView. -B39562.
**	    2. Mapped FIND, TOP and BOTTOM to standard INGRES FRSKEYS. -B39661
**	04-sep-1992 (jpk)
**		add more forward references.
**      10-12-94 (newca01)
**             Added code in dut_ff1_1intro for B44836
**      02-15-95 (brogr02)
**             Changed copyright message
**      03-02-95 (newca01)
**             Addec code in dut_ff1_1intro for B67259
**      21-mar-1995 (sarjo01)
**          Bug 67417: add test for upper case 'IISTAR_CDBS' as well as
**          'iistar_cdbs' to allow query to work correctly in FIPS.
**	08-apr-97 (mcgem01)
**	    Changed CA-OpenIngres to OpenIngres and updated Copyright
**	    notice.
**	10-apr-98 (mcgem01)
**	    Change OpenIngres to Ingres and update copyright notice.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of cv.h and pc.h and added some return types
**	    to eliminate gcc 4.3 warnings.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**/

DUT_STATUS dut_ff2_1ldblist(), dut_ff2_2load_ldb(), dut_ff2_3test_node(),
           dut_ff2_4reload_ldb(), dut_ff2_5test_ldb();
DUT_STATUS dut_ff3_1objlist(), dut_ff3_2load_obj(), dut_ff3_3reload_obj(),
           dut_ff3_4objinfo(), dut_ff3_6remove_object(), dut_ff3_7objects(),
	   dut_ff3_8fixlist();

/*
** Name: dut_ff1_1intro	- Load form 'dut_f1_intro' with data 
**			in a DUT_F1_INTRO pointer
**
** Description:
**	FORM 1 is the introductroy form, which maybe avoided by
**	issuing the starview command with a DDBname as the parameter.
**		options:
**			Go	- enter Star*View with a given DDB.
**			Select	- Select a DDB from list to DDB field.
**			DDBHelp	- List all DDBs in iidbdb of given node.
**			NodeHelp- List nodes.
**			Top, Bottom, Find - operations on DDBlist(table field).
**			Help	- Help for this form.
**			Quit	- Quit Star*View.
**
** Inputs:
**      dut_cb				Control block for starview.
**      dut_errcb                       Error Control block for starview.
**
** Outputs:
**	Returns:
**	    not applicable.
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
**      2-nov-1988 (alexc)
**          Creation.
**     25-jul-1991 (fpang)
**	    - Add activations to update ddbname and ddbnode fields when
**	      user moves up and down in the DdbHelp table field.
**	    - Removed the 'Select' menuitem.
**	    - Fixed Top/Bottom/Find to keep ddbname and ddbnode fields
**	      current when scrolling DdbHelp table field.
**     10-12-94 (newca01)
**          - Added "if" stmt in activate routine for before ddblist
**            for B44836
**     03-02-95 (newca01)
**          - Added second condition on "if" above for b67259
**            Added "if" around every getrow in dut_ff1_1intro().
*/
void
dut_ff1_1intro(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB		*dut_cb;
DUT_ERRCB	*dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    DUT_F1_INTRO    *dut_f1_1intro_p;
    char	    dut_buffer[DDB_2MAXNAME];
    EXEC SQL END DECLARE SECTION;
    char *dut_f1_curr_node = NULL;

    dut_f1_1intro_p = dut_cb->dut_form_intro;
    dut_cb->dut_c9_status	= DUT_F_FORM_1;

    EXEC FRS display dut_f1_intro;
    EXEC FRS initialize;
    EXEC FRS begin;
	/* Set error flag to DUT_ERROR */
	dut_cb->dut_c8_status = DUT_ERROR;
	/* RTI banner */
	EXEC FRS message
/*
	  'INGRES STAR*VIEW -- Copyright (c) 2004 Ingres Corporation';
*/
/*
	  'INGRES StarView -- Copyright (c) 2004 Ingres Corporation';
*/
  'Ingres StarView - Copyright (c) 2004 Ingres Corporation';

	STcopy(dut_cb->dut_c2_nodename, dut_cb->dut_c7_currnode);
	EXEC FRS inittable dut_f1_intro dut_f1_4ddblist read;
	EXEC FRS putform dut_f1_intro
	    (
	    dut_f1_1ddbname	= :dut_cb->dut_c1_ddbname,
	    dut_f1_2nodename	= :dut_cb->dut_c2_nodename,
	    dut_f1_3nodename	= :dut_cb->dut_c2_nodename
	    );
	if (dut_uu5_chkdbname(dut_cb->dut_c1_ddbname) == E_DU_OK)
	{
	    /* If connections is ok to open given DDB, go into LDB form */
	    if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
	    {
		dut_ee1_error(dut_errcb, W_DU1805_SV_CANT_OPEN_DDB, 
                              2, 0, dut_cb->dut_c1_ddbname);
	    }
	    else
	    {
		dut_ff2_1ldblist(dut_cb, dut_errcb);
	    }
	}
    EXEC FRS end;


    EXEC FRS activate before column dut_f1_4ddblist all;
    EXEC FRS begin;
	 /* if stmt added 10/12/94 (newca01) for b44836  */
	 /* second condition in if added 03/02/95 (newca01) for b67259 */
	 if ( (dut_cb->dut_form_intro->dut_f1_4ddblist != NULL) 
	    && (dut_cb->dut_form_intro->dut_f1_4ddblist->dut_t1_1ddbname[0]
	       != EOS) )
	 {
	    EXEC FRS getrow dut_f1_intro dut_f1_4ddblist
		( :dut_cb->dut_c1_ddbname = dut_t1_1ddbname );
	    EXEC FRS putform dut_f1_intro
		( dut_f1_1ddbname = :dut_cb->dut_c1_ddbname );
         }

    EXEC FRS end;

    EXEC FRS activate menuitem 'Go', frskey4;
    EXEC FRS begin;
	EXEC FRS getform dut_f1_intro
	    (
	    :dut_cb->dut_c1_ddbname:dut_cb->dut_null_1 = dut_f1_1ddbname,
	    :dut_cb->dut_c2_nodename:dut_cb->dut_null_2 = dut_f1_2nodename
	    );

	if (dut_cb->dut_null_1 == -1)
	{
	    dut_cb->dut_c1_ddbname[0] = EOS;
	}
	if (dut_cb->dut_null_2 == -1)
	{
	    STcopy(dut_cb->dut_c13_vnodename, dut_cb->dut_c2_nodename);
	}
	if (dut_cb->dut_c1_ddbname[0] != EOS)
	{
	    if (dut_ii2_check_str(DUT_CK_DDBNAME, dut_cb->dut_c1_ddbname,
		dut_errcb)
		!= E_DU_OK)
	    {
		dut_ee1_error(dut_errcb, W_DU1803_SV_BAD_DDBNAME,
				2, 0, dut_cb->dut_c1_ddbname);
	    }
	    else 
	    {
		STprintf(dut_cb->frs_message, "%s::%s", 
			dut_cb->dut_c2_nodename,
			dut_cb->dut_c1_ddbname);
		EXEC FRS message :dut_cb->frs_message;
		if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
		{
		    dut_ee1_error(dut_errcb, W_DU1805_SV_CANT_OPEN_DDB, 
				  2, 0, dut_cb->dut_c1_ddbname);

		}
		else
		{
		    dut_ff2_1ldblist(dut_cb, dut_errcb);
		}
	    }
	}
	else	/* if ddbname field is NULL */
	{
	    dut_ee1_error(dut_errcb, W_DU1801_SV_NO_DDB_SELECTED, 0);
	}
    EXEC FRS end;


    EXEC FRS activate menuitem 'DDBHelp';
    EXEC FRS begin;
	EXEC FRS clear field dut_f1_4ddblist;
	EXEC FRS redisplay;
	EXEC FRS getform dut_f1_intro
	    (
	    :dut_cb->dut_form_intro->dut_f1_2nodename = dut_f1_2nodename
	    );
	if (dut_cb->dut_c0_iidbdbnode[0] == EOS)
	{
	    STcopy(dut_cb->dut_c13_vnodename, dut_cb->dut_c0_iidbdbnode);
	}
	if (dut_cb->dut_form_intro->dut_f1_2nodename[0] != EOS)
	{
	    STcopy(dut_cb->dut_form_intro->dut_f1_2nodename,
			dut_cb->dut_c0_iidbdbnode);
	}
	if (dut_ff1_2load_ddb(dut_cb, dut_errcb, dut_f1_1intro_p) == DUT_ERROR)
	{
	    dut_cb->dut_c8_status = DUT_ERROR;
	}
	else
	{
	 /* if stmt added 03/02/95 (newca01) for b67259 */
	 if ( (dut_cb->dut_form_intro->dut_f1_4ddblist != NULL) 
	    && (dut_cb->dut_form_intro->dut_f1_4ddblist->dut_t1_1ddbname[0]
	       != EOS) )
	 {
	    EXEC FRS putform dut_f1_intro
		( 
		  dut_f1_1ddbname = 
		    :dut_f1_1intro_p->dut_f1_4ddblist->dut_t1_1ddbname
		);
	    EXEC FRS resume field dut_f1_4ddblist;
	 }
	 else 
	 {
	   dut_ee1_error(dut_errcb, W_DU181F_SV_NO_DDBS_FOUND, 0);
	 }
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'Top', frskey5;
    EXEC FRS begin;
	/* following 'if' added around getrow 03-02-95 newca01 b67259 */
	if ((dut_cb->dut_form_intro->dut_f1_4ddblist != NULL)
	   && (dut_cb->dut_form_intro->dut_f1_4ddblist->dut_t1_1ddbname[0]
	       != EOS ))
	{
	dut_uu7_table_top("dut_f1_4ddblist");
        EXEC FRS getrow dut_f1_intro dut_f1_4ddblist
	   ( :dut_cb->dut_c1_ddbname = dut_t1_1ddbname );
	EXEC FRS putform dut_f1_intro
	   ( dut_f1_1ddbname = :dut_cb->dut_c1_ddbname );
	EXEC FRS resume field dut_f1_4ddblist;
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'Bottom', frskey6;
    EXEC FRS begin;
     	/* following 'if' added around getrow 03-02-95 newca01 b67259 */
        if ((dut_cb->dut_form_intro->dut_f1_4ddblist != NULL)
	&& (dut_cb->dut_form_intro->dut_f1_4ddblist->dut_t1_1ddbname[0]
	   != EOS ))
	{
	dut_uu8_table_bottom("dut_f1_4ddblist");
        EXEC FRS getrow dut_f1_intro dut_f1_4ddblist
	   ( :dut_cb->dut_c1_ddbname = dut_t1_1ddbname );
	EXEC FRS putform dut_f1_intro
	   ( dut_f1_1ddbname = :dut_cb->dut_c1_ddbname );
	EXEC FRS resume field dut_f1_4ddblist;
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'Find', frskey7;
    EXEC FRS begin;
	/* following 'if' added around getrow 03-02-95 newca01 b67259 */
	if ((dut_cb->dut_form_intro->dut_f1_4ddblist != NULL)
	&& (dut_cb->dut_form_intro->dut_f1_4ddblist->dut_t1_1ddbname[0]
	    != EOS ))
        {
	dut_uu9_table_find(dut_cb, dut_errcb, DUT_M00_DDBNAME,
			"dut_f1_4ddblist",
			NULL);
        EXEC FRS getrow dut_f1_intro dut_f1_4ddblist
	   ( :dut_cb->dut_c1_ddbname = dut_t1_1ddbname );
	EXEC FRS putform dut_f1_intro
	   ( dut_f1_1ddbname = :dut_cb->dut_c1_ddbname );
	EXEC FRS resume field dut_f1_4ddblist;
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'Help', frskey1;
    EXEC FRS begin;
	EXEC FRS HELP_FRS
	    (
	    subject	= 'Distributed Database Help',
	    file	= :dut_cb->dut_helpcb->dut_hf1_1name
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'Quit', frskey2;
    EXEC FRS begin;
	dut_cb->dut_c8_status = E_DU_OK;
	dut_cb->dut_c9_status = DUT_F_NULL;
	EXEC FRS breakdisplay;
    EXEC FRS end;

    EXEC FRS finalize;
    EXEC FRS clear screen;
}


/*
** Name: dut_ff1_2load_ddb	- Load DDB information from iidbdb.
**
** Description:
**		This function loads information about DDBs from 'iistar_cdbs'
**	in iidbdb into a table field in form 'dut_f1_intro' and into an 
**	dynamically allocated array of DUT_T1_DDBLIST.
**
** Inputs:
**      dut_cb				Control block for starview.
**      dut_errcb                       Error Control block for starview.
**
** Outputs:
**	Returns:
**	    not applicable.
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
**      2-nov-1988 (alexc)
**          Creation.
**     25-jul=1991 (fpang)
**	    Explicitly return E_DU_OK if successful.
*/
DUT_STATUS
dut_ff1_2load_ddb(dut_cb, dut_errcb, dut_f1_intro_p)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
DUT_F1_INTRO	*dut_f1_intro_p;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    DUT_T1_DDBLIST *dut_f1_4ddblist_p;
    DUT_T1_DDBLIST *dut_f1_0ddblist_curr_p;
    i4		dut_f1_0ddb_count;
    char	dut_msg[DB_LONGMSG];
    EXEC SQL END DECLARE SECTION;
    i4		dut_f1_0ddb_fill;

    dut_f1_0ddb_count = 0;
    dut_f1_0ddb_fill  = 0;
    dut_f1_4ddblist_p = dut_f1_intro_p->dut_f1_4ddblist;

    STprintf(dut_msg, DUT_M01_LOADDDB,
	dut_cb->dut_c0_iidbdbnode);
    EXEC FRS message :dut_msg;

    if (dut_uu2_dbop(DUT_IIDBDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU180A_SV_CANT_OPEN_IIDBDB, 0);
	return(DUT_ERROR);
    }

    EXEC SQL select count(*)
	into :dut_f1_0ddb_count
	from iitables
	where table_name = 'iistar_cdbs'
	or    table_name = 'IISTAR_CDBS';

    if (dut_f1_0ddb_count == 0)
    {
	/* This means there is no table call iistar_cdbs in remote iidbdb */
	/* Remote installation does not run INGRES/STAR */
	return(DUT_ERROR);
    }
    EXEC SQL select count(*)
	into :dut_f1_0ddb_count
	from iistar_cdbs;

    if (dut_f1_0ddb_count > dut_cb->dut_form_intro->dut_f1_0ddb_size)
    {
	MEfree(dut_f1_4ddblist_p);
	dut_f1_intro_p->dut_f1_4ddblist = NULL;
	dut_f1_4ddblist_p = NULL;
    }

    if (dut_f1_4ddblist_p == NULL)
    {
	dut_f1_4ddblist_p = (DUT_T1_DDBLIST *)MEreqmem(0,
		(dut_f1_0ddb_count + DUT_ME_EXTRA) * sizeof(DUT_T1_DDBLIST),
		TRUE, NULL);
	dut_f1_intro_p->dut_f1_4ddblist = dut_f1_4ddblist_p;
	dut_cb->dut_form_intro->dut_f1_0ddb_size
		= dut_f1_0ddb_count + DUT_ME_EXTRA;
    }

    dut_f1_0ddblist_curr_p		= dut_f1_4ddblist_p;

    EXEC FRS getform dut_f1_intro
	(
	:dut_f1_intro_p->dut_f1_2nodename = dut_f1_2nodename,
	:dut_f1_intro_p->dut_f1_3nodename = dut_f1_3nodename	
	);

    STcopy(dut_f1_intro_p->dut_f1_2nodename, dut_cb->dut_c0_iidbdbnode);
    CVlower(dut_cb->dut_c0_iidbdbnode);

    EXEC SQL 
	SELECT distinct
	    lowercase(trim(ddb_name)),
	    lowercase(trim(ddb_owner)),
	    lowercase(trim(cdb_name)),
	    lowercase(trim(cdb_node))
	into
	    :dut_f1_0ddblist_curr_p->dut_t1_1ddbname,
	    :dut_f1_0ddblist_curr_p->dut_t1_2ddbowner,
	    :dut_f1_0ddblist_curr_p->dut_3cdbname,
	    :dut_f1_0ddblist_curr_p->dut_4nodename
	from iistar_cdbs
	where lowercase(cdb_node) = :dut_cb->dut_c0_iidbdbnode
	order by 1;
    EXEC SQL BEGIN;
	EXEC FRS loadtable dut_f1_intro dut_f1_4ddblist
	    (
	    dut_t1_1ddbname	= :dut_f1_0ddblist_curr_p->dut_t1_1ddbname,
	    dut_t1_2ddbowner	= :dut_f1_0ddblist_curr_p->dut_t1_2ddbowner
	    );

	dut_f1_0ddb_fill++;
	dut_f1_0ddblist_curr_p++;

	if (sqlca.sqlcode < 0)
	{
	    dut_cb->dut_c8_status = !E_DU_OK;
	    EXEC SQL endselect;
	}
    EXEC SQL END;

    dut_cb->dut_form_intro->dut_f1_0ddb_num_entry = dut_f1_0ddb_fill;

    STcopy(dut_f1_intro_p->dut_f1_2nodename, dut_f1_intro_p->dut_f1_3nodename);

    EXEC FRS putform dut_f1_intro
	(
	dut_f1_3nodename = :dut_f1_intro_p->dut_f1_3nodename
	);

    return ( E_DU_OK );
}
    

/*
** Name: dut_ff2_1ldblist	- Form 2, 'dut_f2_ldblist'.
**
** Description:
**      FORM 2 is the form that give LDB information for a DDB.
**
** Inputs:
**      dut_cb				Control block for starview.
**      dut_errcb                       Error Control block for starview.
**
** Outputs:
**	Returns:
**	    not applicable.
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
**      28-nov-1988 (alexc)
**          Creation.
*/
DUT_STATUS
dut_ff2_1ldblist(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    DUT_F2_LDBLIST  *dut_f2_2ldblist_p;
    DUT_T2_LDBLIST  *dut_t2_ldblist_p;
    char  dut_dbname[DDB_2MAXNAME];
    EXEC SQL END DECLARE SECTION;

    dut_f2_2ldblist_p = dut_cb->dut_form_ldblist;
    dut_t2_ldblist_p  = dut_f2_2ldblist_p->dut_f2_3ldblist_curr_p;
    dut_cb->dut_c8_status = E_DU_OK;
    dut_cb->dut_c9_status = DUT_F_FORM_3;

    if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU1805_SV_CANT_OPEN_DDB, 2, 0, 
                      dut_cb->dut_c1_ddbname);
	dut_cb->dut_c8_status = DUT_ERROR;
	return(DUT_ERROR);
    }

    EXEC FRS display dut_f2_ldblist;

    EXEC FRS initialize;
    EXEC FRS begin;
	dut_cb->dut_c8_status = !E_DU_OK;

	EXEC FRS inittable dut_f2_ldblist dut_f2_3ldblist read;
	EXEC FRS putform dut_f2_ldblist
	    (
		dut_f2_1ddbname	= :dut_cb->dut_c1_ddbname,
		dut_f2_2nodename= :dut_cb->dut_c2_nodename
	    );
	dut_cb->dut_c8_status = E_DU_OK;
	dut_ff2_2load_ldb(dut_cb, dut_errcb, dut_f2_2ldblist_p);
	if (dut_cb->dut_c8_status == DUT_ERROR)
	{
	    EXEC FRS breakdisplay;
	    dut_cb->dut_c9_status = DUT_F_FORM_1;
	    dut_cb->dut_c8_status = DUT_ERROR;
	    return(DUT_ERROR);
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'ListObj';
    EXEC FRS begin;
	dut_cb->dut_form_ldblist->dut_f2_0modify = FALSE;
	EXEC FRS getrow dut_f2_ldblist dut_f2_3ldblist
	    (
	    :dut_t2_ldblist_p->dut_t2_1status	= dut_t2_1status,
	    :dut_t2_ldblist_p->dut_t2_2nodename	= dut_t2_2nodename,
	    :dut_t2_ldblist_p->dut_t2_3ldbname	= dut_t2_3ldbname,
	    :dut_t2_ldblist_p->dut_t2_4ldbtype	= dut_t2_4ldbtype
	    );
	dut_ff3_1objlist(dut_cb, dut_errcb, dut_f2_2ldblist_p);
	if (dut_cb->dut_c8_status != E_DU_OK)
	{
	    dut_ee1_error(dut_errcb, W_DU1808_SV_CANT_GET_LDB_INFO, 2,
		0, dut_cb->dut_c3_ldbname);
	}
	if (dut_cb->dut_form_ldblist->dut_f2_0modify == TRUE)
	{
	    dut_cb->dut_form_ldblist->dut_f2_0modify = FALSE;
	    dut_ff2_4reload_ldb(dut_cb, dut_errcb);
	}
	dut_uu23_reset_ldblist(dut_cb, dut_errcb);
	dut_ff2_4reload_ldb(dut_cb, dut_errcb);
    EXEC FRS end;

    EXEC FRS activate menuitem 'LDBAttr';
    EXEC FRS begin;
	EXEC FRS getrow dut_f2_ldblist dut_f2_3ldblist
	    (
	    :dut_t2_ldblist_p->dut_t2_1status	= dut_t2_1status,
	    :dut_t2_ldblist_p->dut_t2_2nodename	= dut_t2_2nodename,
	    :dut_t2_ldblist_p->dut_t2_3ldbname	= dut_t2_3ldbname,
	    :dut_t2_ldblist_p->dut_t2_4ldbtype	= dut_t2_4ldbtype
	    );
	STcopy(dut_t2_ldblist_p->dut_t2_2nodename, 
		dut_cb->dut_popup_dbattr->dut_p7_0nodename);
	STcopy(dut_t2_ldblist_p->dut_t2_3ldbname, 
		dut_cb->dut_popup_dbattr->dut_p7_1dbname);
	dut_pp7_1dbattr(dut_cb, dut_errcb);
    EXEC FRS end;

    EXEC FRS activate menuitem 'TestNode';
    EXEC FRS begin;
	dut_ff2_3test_node(dut_cb, dut_errcb);
	EXEC FRS resume menu;
    EXEC FRS end;

    EXEC FRS activate menuitem 'TestLDB';
    EXEC FRS begin;
	dut_ff2_5test_ldb(dut_cb, dut_errcb);
	EXEC FRS resume menu;
    EXEC FRS end;

    EXEC FRS activate menuitem 'SQL';
    EXEC FRS begin;
	if ((dut_cb->dut_c2_nodename[0] == EOS) ||
	    !STcompare(dut_cb->dut_c2_nodename, dut_cb->dut_c13_vnodename))
	{
	    STprintf(dut_dbname, "%s/star", dut_cb->dut_c1_ddbname);
	}
	else
	{
	    STprintf(dut_dbname, "%s::%s/star",
			dut_cb->dut_c2_nodename,
			dut_cb->dut_c1_ddbname);
	}
	STprintf(dut_cb->dut_c0_buffer, "ISQL %s", dut_dbname);
	EXEC FRS message :dut_cb->dut_c0_buffer;
	EXEC SQL commit;
	EXEC SQL call isql (database = :dut_dbname);
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
	    EXEC SQL select dbmsinfo('database') into :dut_cb->dut_c0_buffer;
	    STtrmwhite(dut_cb->dut_c0_buffer);
	    if (STcompare(dut_cb->dut_c0_buffer, dut_cb->dut_c1_ddbname))
	    {
		/* After direct disconnect, the dbname is not ddb */
		PCexit(0);
	    }
	}
	dut_cb->dut_c7_status = DUT_S_CONNECT_DDB;
	EXEC SQL commit;
	EXEC FRS redisplay;
    EXEC FRS end;

    EXEC FRS activate menuitem 'Tables';
    EXEC FRS begin;
	if ((dut_cb->dut_c2_nodename[0] == EOS) ||
	    !STcompare(dut_cb->dut_c2_nodename, dut_cb->dut_c13_vnodename))
	{
	    STprintf(dut_dbname, "%s/star", dut_cb->dut_c1_ddbname);
	}
	else
	{
	    STprintf(dut_dbname, "%s::%s/star",
			dut_cb->dut_c2_nodename,
			dut_cb->dut_c1_ddbname);
	}
	STprintf(dut_cb->frs_message, "TABLES %s", dut_dbname);
	EXEC FRS message :dut_cb->frs_message;
	EXEC SQL commit;
	EXEC SQL call tables (database = :dut_dbname);
	EXEC SQL commit;
	EXEC SQL select dbmsinfo('database') into :dut_cb->dut_c0_buffer;
	STtrmwhite(dut_cb->dut_c0_buffer);
	if (STcompare(dut_cb->dut_c0_buffer, dut_cb->dut_c1_ddbname))
	{
	    /* After direct disconnect, the dbname is not ddb */
	    PCexit(0);
	}
	EXEC FRS redisplay;
    EXEC FRS end;

    EXEC FRS activate menuitem 'Top', frskey5;
    EXEC FRS begin;
	dut_uu7_table_top(NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Bottom', frskey6;
    EXEC FRS begin;
	dut_uu8_table_bottom(NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Help', frskey1;
    EXEC FRS begin;
	EXEC FRS HELP_FRS
	    (
	    subject	= 'Distributed Database Help',
	    file	= :dut_cb->dut_helpcb->dut_hf2_1name
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	dut_cb->dut_c8_status = E_DU_OK;
	/* Need to clean all LDB, and CDB data in 'dut_cb' */
	dut_cb->dut_c1_ddbname[0] = EOS;
	dut_cb->dut_c2_nodename[0] = EOS;
	dut_cb->dut_c3_ldbname[0] = EOS;
	dut_cb->dut_c4_ldbnode[0] = EOS;
	dut_cb->dut_c5_cdbname[0] = EOS;
	dut_cb->dut_c6_cdbnode[0] = EOS;
	dut_cb->dut_c8_status   = E_DU_OK;
	dut_cb->dut_c9_status   = DUT_F_FORM_1;
	EXEC FRS breakdisplay;
    EXEC FRS end;

}


/*
** Name: dur_ff2_2load_ldb - Loads Local databases from iidd_ddb_ldbids.
**
** Description:
**      Loads local database information from iidd_ddb_ldbids.  This means 
**      all local databases that have objects registered in this Distributed
**      database.
**
** Inputs:
**      dut_cb                          StarView main control block.
**      dut_errcb                       StarView error control block.
**      dut_f2_ldblist_p                pointer to array of LDBs.
**
** Outputs:
**      None.
**	Returns:
**	    None
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      5-dec-1988 (alexc)
**          Creation.
*/
DUT_STATUS
dut_ff2_2load_ldb(dut_cb, dut_errcb, dut_f2_ldblist_p)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
DUT_F2_LDBLIST	*dut_f2_ldblist_p;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    DUT_T2_LDBLIST	*dut_f2_3ldblist_p;
    DUT_T2_LDBLIST	*dut_f2_0ldblist_curr_p;
    i4			dut_f2_0ldb_count;
    char		dut_temp_ddbname[DDB_MAXNAME + 4];
    char		dut_temp_ddbnode[DDB_MAXNAME + 4];
    EXEC SQL END DECLARE SECTION;
    i4			dut_f2_0ldb_fill;

    dut_f2_ldblist_p->dut_f2_0modify = FALSE;
    dut_f2_0ldb_count = 0;
    dut_f2_0ldb_fill  = 0;
    dut_f2_3ldblist_p = (DUT_T2_LDBLIST *)dut_f2_ldblist_p->dut_f2_3ldblist;

    EXEC FRS getform dut_f2_ldblist
	(
	:dut_temp_ddbname = dut_f2_1ddbname,
	:dut_temp_ddbnode = dut_f2_2nodename
	);

    if (!STcompare(dut_cb->dut_c1_ddbname, dut_temp_ddbname) &&
	!STcompare(dut_cb->dut_c2_nodename, dut_temp_ddbnode))
    {
	EXEC FRS clear field all;
    }
    else
    {
	return(E_DU_OK);
    }

    EXEC FRS putform dut_f2_ldblist
	(
	dut_f2_1ddbname	 = :dut_cb->dut_c1_ddbname,
	dut_f2_2nodename = :dut_cb->dut_c2_nodename
	);

    if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU1805_SV_CANT_OPEN_DDB, 2, 0, 
			dut_cb->dut_c1_ddbname);
	dut_cb->dut_c8_status = DUT_ERROR;
	return(DUT_ERROR);
    }
    EXEC SQL select 
	count(*) into :dut_f2_0ldb_count
	from iiddb_ldbids;

    if (dut_f2_0ldb_count > dut_cb->dut_form_ldblist->dut_f2_0ldb_size)
    {
	MEfree(dut_f2_3ldblist_p);
	dut_f2_ldblist_p->dut_f2_3ldblist = NULL;
	dut_f2_3ldblist_p = NULL;
    }

    if (dut_f2_3ldblist_p == NULL)
    {
	dut_f2_3ldblist_p = (DUT_T2_LDBLIST *)MEreqmem(0,
		(dut_f2_0ldb_count + DUT_ME_EXTRA) * sizeof(DUT_T2_LDBLIST),
		TRUE, NULL);
	dut_f2_ldblist_p->dut_f2_3ldblist = dut_f2_3ldblist_p;
	dut_cb->dut_form_ldblist->dut_f2_0ldb_size 
		= dut_f2_0ldb_count + DUT_ME_EXTRA;
    }

    dut_f2_0ldblist_curr_p	= dut_f2_3ldblist_p;

    EXEC SQL 
	SELECT distinct
	    ldb_id,
	    trim(ldb_node),
	    trim(ldb_database),
	    trim(ldb_dbms)
	into
	    :dut_f2_0ldblist_curr_p->dut_t2_0ldbid,
	    :dut_f2_0ldblist_curr_p->dut_t2_2nodename,
	    :dut_f2_0ldblist_curr_p->dut_t2_3ldbname,
	    :dut_f2_0ldblist_curr_p->dut_t2_4ldbtype
	from iiddb_ldbids
	order by 1;
    EXEC SQL begin;
	STcopy("?", dut_f2_0ldblist_curr_p->dut_t2_1status);

	dut_f2_0ldb_fill++;
	dut_f2_0ldblist_curr_p++;

	if (sqlca.sqlcode < 0)
	{
	    dut_cb->dut_c8_status = !E_DU_OK;
	    EXEC SQL endselect;
	    dut_ee1_error(dut_errcb, W_DU1807_SV_CANT_GET_DDB_INFO, 2,
		0, dut_cb->dut_c1_ddbname);
	}
    EXEC SQL end;

    dut_cb->dut_form_ldblist->dut_f2_0ldb_num_entry = dut_f2_0ldb_fill;

    if (dut_f2_0ldblist_curr_p->dut_t2_0ldbid == 1)
    {
	STcopy(dut_f2_0ldblist_curr_p->dut_t2_3ldbname,
		dut_cb->dut_c5_cdbname);
	STcopy(dut_f2_0ldblist_curr_p->dut_t2_2nodename,
		dut_cb->dut_c6_cdbnode);
    }

    dut_uu13_buble_sort(dut_cb->dut_form_ldblist->dut_f2_3ldblist,
		sizeof(DUT_T2_LDBLIST), DDB_MAXNAME, 
		dut_cb->dut_form_ldblist->dut_f2_0ldb_num_entry);

    dut_ff2_4reload_ldb(dut_cb, dut_errcb);

    if (sqlca.sqlcode < 0)
    {
	int sqlcode = sqlca.sqlcode;

	dut_ee1_error(dut_errcb, W_DU1808_SV_CANT_GET_LDB_INFO, 2,
		0, dut_cb->dut_c3_ldbname);
	dut_cb->dut_c8_status = DUT_ERROR;
	return(sqlcode);
    }
    dut_cb->dut_c8_status = E_DU_OK;
    return(E_DU_OK);
}


/*
** Name: dut_ff2_3test_node - Test connection to node by connecting to iidbdb
**				of the nodes.
**
** Description:
**      Test node options: SingleNode, AllNodes, Newnode. 
**
** Inputs:
**      dut_cb->dut_form_ldblist    - pointer to list of local database, and
**				    node.
**
** Outputs:
**	dut_cb->dut_f2_3ldblist->dut_t2_1status	- '?' -> ['up'|'down'].
**
**	Returns:
**	    E_DU_OK
**	Exceptions:
**
** Side Effects:
**
** History:
**      5-dec-1988 (alexc)
**          Creation.
**     25-jul-1991 (fpang)
**	    - Cleanup associations after AllLdbs and AllNodes sub menu
**	      selections only. Don't do it when 'End' is selected
**	      because connections were never made.
**	    - Nicer text when displaying node/ldb accessibility message.
*/
DUT_STATUS
dut_ff2_3test_node(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    char	new_nodename[DDB_MAXNAME];
    i4		dut_f2_curr_row;
    DUT_T2_LDBLIST	*dut_t2_curr_p;
    EXEC SQL END DECLARE SECTION;
    char dut_buffer[DB_2MAXNAME];

    dut_cb->dut_on_error_exit = FALSE;
    dut_cb->dut_on_error_noop = TRUE;
    dut_t2_curr_p = dut_cb->dut_form_ldblist->dut_f2_3ldblist_curr_p;

    EXEC FRS submenu;

    EXEC FRS activate menuitem 'SingleNode';
    EXEC FRS begin;
	if (!dut_uu12_empty_table(dut_cb, dut_errcb, "dut_f2_3ldblist"))
	{
	    dut_ee1_error(dut_errcb, W_DU1812_SV_EMPTY_TABLE, 0);
	}
        else
	{
	    EXEC FRS putrow dut_f2_ldblist dut_f2_3ldblist
		(
		dut_t2_1status = '?'
		);
	    EXEC FRS redisplay;
	    EXEC FRS getrow dut_f2_ldblist dut_f2_3ldblist
		(
		:dut_t2_curr_p->dut_t2_1status	= dut_t2_1status,
		:dut_t2_curr_p->dut_t2_2nodename= dut_t2_2nodename,
		:dut_t2_curr_p->dut_t2_3ldbname	= dut_t2_3ldbname,
		:dut_t2_curr_p->dut_t2_4ldbtype	= dut_t2_4ldbtype
		);
	    EXEC FRS inquire_frs table dut_f2_ldblist
	        (
		:dut_f2_curr_row = rowno(dut_f2_3ldblist)
		);
	    dut_uu21_test_node(dut_cb, dut_errcb,
				dut_t2_curr_p->dut_t2_2nodename);
	    dut_ff2_4reload_ldb(dut_cb, dut_errcb);
	    EXEC FRS scroll dut_f2_ldblist dut_f2_3ldblist
		to :dut_f2_curr_row;
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'AllNodes';
    EXEC FRS begin;
	dut_uu20_test_nodelist(dut_cb, dut_errcb);
	EXEC FRS message 'Cleaning up associations. . .';
	dut_uu2_dbop(DUT_DDBCLOSE, dut_cb, dut_errcb);
	if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
	{
	    dut_ee1_error(dut_errcb, W_DU1805_SV_CANT_OPEN_DDB, 2, 0,
			  dut_cb->dut_c1_ddbname);
	}
	dut_ff2_4reload_ldb(dut_cb, dut_errcb);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Help', frskey1;
    EXEC FRS begin;
	EXEC FRS HELP_FRS
	    (
	    subject	= 'Distributed Database Help',
	    file	= :dut_cb->dut_helpcb->dut_hf2_2name
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
    EXEC FRS end;
}


/*
** Name: dur_ff2_4reload_ldb - Reload ldb list from array to screen again
**			    without reloading the array from database.
**
** Description:
**      Reload LDB from dut_cb->dut_form_ldblist->dut_f2_3ldblist(array)
**	    to table field in dut_f2_ldblist. 
**
** Inputs:
**      dut_cb	                         .dut_form_ldblist
**					    pointer to form structure.
**
** Outputs:
**      none. #41$
**	Returns:
**	    E_DU_OK
**	    DUT_ERROR
**	Exceptions:
**
** Side Effects:
**
** History:
**      5-dec-1988 (alexc)
**          Creation.
*/
DUT_STATUS
dut_ff2_4reload_ldb(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    /* Debugging routine */
    EXEC SQL begin declare section;
    DUT_T2_LDBLIST	*t2_p;
    DUT_T2_LDBLIST	t2_0buffer;
    DUT_T2_LDBLIST	t2_1buffer;
    i4			t2_row;
    EXEC SQL end declare section;
    i4			t2_cnt;
    char		buffer[100];

    t2_p	= dut_cb->dut_form_ldblist->dut_f2_3ldblist;
    t2_cnt	= dut_cb->dut_form_ldblist->dut_f2_0ldb_num_entry;
    t2_row	= 0;

    if (dut_uu12_empty_table(dut_cb, dut_errcb, "dut_f2_3ldblist"))
    {
        EXEC FRS getrow dut_f2_ldblist dut_f2_3ldblist
	    (
	    :t2_0buffer.dut_t2_1status	= dut_t2_1status,
	    :t2_0buffer.dut_t2_2nodename= dut_t2_2nodename,
	    :t2_0buffer.dut_t2_3ldbname	= dut_t2_3ldbname,
	    :t2_0buffer.dut_t2_4ldbtype	= dut_t2_4ldbtype
	    );
	t2_row = 1;
    }
    EXEC FRS clear field dut_f2_3ldblist;

    while (t2_cnt--)
    {
	EXEC FRS loadtable dut_f2_ldblist dut_f2_3ldblist
	    (
	    dut_t2_1status	= :t2_p->dut_t2_1status,
	    dut_t2_2nodename	= :t2_p->dut_t2_2nodename,
	    dut_t2_3ldbname	= :t2_p->dut_t2_3ldbname,
	    dut_t2_4ldbtype	= :t2_p->dut_t2_4ldbtype
	    );
	t2_p++;
    }

    /* Move cursor back to where it was */
    if (t2_row == 0)
    {
	EXEC FRS unloadtable dut_f2_ldblist dut_f2_3ldblist
	    (
	    :t2_1buffer.dut_t2_1status	= dut_t2_1status,
	    :t2_1buffer.dut_t2_2nodename= dut_t2_2nodename,
	    :t2_1buffer.dut_t2_3ldbname	= dut_t2_3ldbname,
	    :t2_1buffer.dut_t2_4ldbtype	= dut_t2_4ldbtype,
	    :t2_row			= _RECORD
	    );
	EXEC FRS begin;
	    if (!STcompare(t2_0buffer.dut_t2_2nodename,
			t2_1buffer.dut_t2_2nodename)
			&&
		!STcompare(t2_0buffer.dut_t2_3ldbname,
			t2_1buffer.dut_t2_3ldbname))
	    {
		EXEC FRS scroll dut_f2_ldblist dut_f2_3ldblist
		    to :t2_row;
	    }
	EXEC FRS end;
    }
    else
    {
	EXEC FRS scroll dut_f2_ldblist dut_f2_3ldblist
	    to :t2_row;
    }
    return(E_DU_OK);
}


/*
** Name: dur_ff2_5test_ldb - Test connection to local database.
**
** Description:
**      Driver routine to test connection to local database by doing
**	    direct connects to the local database.  User can choose the
**	    following options in testing LDB connections: 
**		SingleLDB   - test single local database. 
**		AllLDBs	    - test all local databases. 
**		NewLDB	    - Allow user to enter a database name. 
**
** Inputs:
**      none #41$
**
** Outputs:
**      none. #41$
**	Returns:
**	    E_DU_OK
**	    DUT_ERROR
**	Exceptions:
**
** Side Effects:
**
** History:
**      23-mar-1988 (alexc)
**          Creation.
**     25-jul-1991 (fpang)
**	    - Cleanup associations after AllLdbs and AllNodes sub menu
**	      selections only. Don't do it when 'End' is selected
**	      because connections were never made.
**	    - Nicer text when displaying node/ldb accessibility message.
*/
DUT_STATUS
dut_ff2_5test_ldb(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    char	new_nodename[DDB_MAXNAME];
    char	new_ldbname[DDB_MAXNAME];
    i4		dut_f2_curr_row;
    DUT_T2_LDBLIST	*dut_t2_curr_p;
    EXEC SQL END DECLARE SECTION;
    char dut_buffer[DB_2MAXNAME];

    dut_t2_curr_p = dut_cb->dut_form_ldblist->dut_f2_3ldblist_curr_p;

    EXEC FRS submenu;

    EXEC FRS activate menuitem 'SingleLDB';
    EXEC FRS begin;
	if (!dut_uu12_empty_table(dut_cb, dut_errcb, "dut_f2_3ldblist"))
	{
	    dut_ee1_error(dut_errcb, W_DU1812_SV_EMPTY_TABLE, 0);
	}
        else
        {
	    EXEC FRS putrow dut_f2_ldblist dut_f2_3ldblist
		(
		dut_t2_1status = '?'
		);
	    EXEC FRS redisplay;
	    EXEC FRS getrow dut_f2_ldblist dut_f2_3ldblist
		(
		:dut_t2_curr_p->dut_t2_1status	= dut_t2_1status,
		:dut_t2_curr_p->dut_t2_2nodename= dut_t2_2nodename,
		:dut_t2_curr_p->dut_t2_3ldbname	= dut_t2_3ldbname,
		:dut_t2_curr_p->dut_t2_4ldbtype	= dut_t2_4ldbtype
		);
	    EXEC FRS inquire_frs table dut_f2_ldblist
	        (
		:dut_f2_curr_row = rowno(dut_f2_3ldblist)
		);
	    dut_uu19_test_ldb(dut_cb, dut_errcb, dut_t2_curr_p);
	    dut_ff2_4reload_ldb(dut_cb, dut_errcb);
	    EXEC FRS scroll dut_f2_ldblist dut_f2_3ldblist
		to :dut_f2_curr_row;
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'AllLDBs';
    EXEC FRS begin;
	dut_uu22_test_ldblist(dut_cb, dut_errcb);
	/* Disconnect from DDB and reconnect back to DDB
	** to clean up associations.
	*/
	EXEC FRS message 'Cleaning up associations. . .';
	dut_uu2_dbop(DUT_DDBCLOSE, dut_cb, dut_errcb);
	if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
	{
	    dut_ee1_error(dut_errcb, W_DU1805_SV_CANT_OPEN_DDB, 2, 0, 
			dut_cb->dut_c1_ddbname);
	}
	dut_ff2_4reload_ldb(dut_cb, dut_errcb);
    EXEC FRS end;

    EXEC FRS activate menuitem 'NewLDB';
    EXEC FRS begin;
	dut_cb->dut_on_error_exit = FALSE;
	dut_cb->dut_on_error_noop = TRUE;
	EXEC FRS prompt 
	    (
	    'Enter New Node Name:',
	    :new_nodename
	    );
	if (new_nodename[0] == EOS)
	    return(DUT_ERROR);
	EXEC FRS prompt 
	    (
	    'Enter New LDB Name:',
	    :new_ldbname
	    );
	if (new_ldbname[0] == EOS)
	    return(DUT_ERROR);
	STcopy(new_nodename, dut_cb->dut_c4_ldbnode);
	STcopy(new_ldbname, dut_cb->dut_c3_ldbname);

	if (dut_uu2_dbop(DUT_DIRECT_CONNECT, dut_cb, dut_errcb) != E_DU_OK)
	{
	    /* Not accessible */
	    STprintf(dut_buffer,DUT_M02_NOTACCESS,new_nodename,new_ldbname);
	}
	else
	{
	    /* Accessible */
	    STprintf(dut_buffer,DUT_M03_ACCESS,new_nodename,new_ldbname);
	}
	dut_pp11_1perror(dut_buffer);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Help', frskey1;
    EXEC FRS begin;
	EXEC FRS HELP_FRS
	    (
	    subject	= 'Distributed Database Help',
	    file	= :dut_cb->dut_helpcb->dut_hf2_5name
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
    EXEC FRS end;
}


/*
** Name: dur_ff3_1objlist - List distributed objects in distributed database.
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
**      5-dec-1988 (alexc)
**          Creation.
**     25-jul-1991 (fpang)
**	    - Fix B38733, indexes still displayed when registered object
**	      with indexes is removed. The reverse was also true, when
**	      local object with indexes was registered, indexes were not
**	      displayed.
**	    - Changed Browse/Remove to reload the objlist because they may
**	      may have changed as a result of the Register/Remove.
**	      Added dut_ff3_7objects() and dut_ff3_8fixlist() to support
**	      reloading of objlist.
*/
DUT_STATUS
dut_ff3_1objlist(dut_cb, dut_errcb, dut_f2_ldblist_p)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
DUT_F2_LDBLIST	*dut_f2_ldblist_p;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    DUT_F3_OBJLIST	*dut_f3_objlist_p;
    DUT_T3_OBJLIST	*dut_t3_objlist_p;
    DUT_P3_REGISTER	*dut_p3_register_p;
    EXEC SQL END DECLARE SECTION;

    dut_cb->dut_c9_status = DUT_F_FORM_3;
    dut_f3_objlist_p	= dut_cb->dut_form_objlist;
    dut_p3_register_p	= dut_cb->dut_popup_register;
    dut_t3_objlist_p	= dut_cb->dut_form_objlist->dut_f3_9objlist_curr_p;

    STcopy(dut_f2_ldblist_p->dut_f2_3ldblist_curr_p->dut_t2_2nodename, 
		dut_cb->dut_c4_ldbnode);
    STcopy(dut_f2_ldblist_p->dut_f2_3ldblist_curr_p->dut_t2_3ldbname, 
		dut_cb->dut_c3_ldbname);
    EXEC FRS display dut_f3_objlist;

    EXEC FRS initialize;
    EXEC FRS begin;
	EXEC FRS inittable dut_f3_objlist dut_f3_9objlist read;
	dut_cb->dut_c8_status = !E_DU_OK;
	EXEC FRS putform dut_f3_objlist
	    (
	    dut_f3_8ddbname	= :dut_cb->dut_c1_ddbname,
	    dut_f3_1nodename	= '*',
	    dut_f3_2dbname	= '*',
	    dut_f3_3owner	= '*',
	    dut_f3_5show_view	= 'y',
	    dut_f3_6show_table	= 'y',
	    dut_f3_7show_system	= 'n'
	    );
	dut_ff3_2load_obj(dut_cb, dut_errcb);
    EXEC FRS end;

    EXEC FRS activate menuitem 'ObjAttr';
    EXEC FRS begin;
	if (!dut_uu12_empty_table(dut_cb, dut_errcb, "dut_f3_9objlist"))
	{
	    dut_ee1_error(dut_errcb, W_DU1812_SV_EMPTY_TABLE, 0);
	}
	else
	{
	    dut_ff3_4objinfo(dut_cb, dut_errcb);
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'Browse';
    EXEC FRS begin;
	dut_cb->dut_c8_status = E_DU_OK;
	dut_pp14_1browse(dut_cb, dut_errcb);
	if (dut_cb->dut_c8_status == DUT_OBJLIST_CHANGED)
	{
	    dut_ff3_7objects(dut_cb, dut_errcb);
            dut_ff3_3reload_obj(dut_cb, dut_errcb);
	    dut_cb->dut_c8_status = E_DU_OK;
	}

	dut_uu17_newldb(dut_cb, dut_errcb);

	EXEC FRS redisplay;
    EXEC FRS end;

    EXEC FRS activate menuitem 'Remove';
    EXEC FRS begin;

	if (!dut_uu12_empty_table(dut_cb, dut_errcb, "dut_f3_9objlist"))
	{
	    dut_ee1_error(dut_errcb, W_DU1812_SV_EMPTY_TABLE, 0);
	}
	else
	{
	    EXEC FRS getrow dut_f3_objlist dut_f3_9objlist
		(
		:dut_t3_objlist_p->dut_t3_0objowner= dut_t3_0objowner,
		:dut_t3_objlist_p->dut_t3_1objname = dut_t3_1objname,
		:dut_t3_objlist_p->dut_t3_2objtype = dut_t3_2objtype
		);
	    dut_ff3_6remove_object(dut_cb, dut_errcb);
	}
    EXEC FRS end;

    EXEC FRS activate menuitem 'Criteria';
    EXEC FRS begin;

	EXEC FRS getform dut_f3_objlist
	    (
	    :dut_f3_objlist_p->dut_f3_1nodename		= dut_f3_1nodename,
	    :dut_f3_objlist_p->dut_f3_2dbname		= dut_f3_2dbname,
	    :dut_f3_objlist_p->dut_f3_3owner		= dut_f3_3owner,
	    :dut_f3_objlist_p->dut_f3_5show_view	= dut_f3_5show_view,
	    :dut_f3_objlist_p->dut_f3_6show_table	= dut_f3_6show_table,
	    :dut_f3_objlist_p->dut_f3_7show_system	= dut_f3_7show_system
	    );


	dut_pp2_1criteria(dut_cb, dut_errcb, dut_f3_objlist_p);

	EXEC FRS putform dut_f3_objlist
	    (
	    dut_f3_1nodename	= :dut_f3_objlist_p->dut_f3_1nodename,
	    dut_f3_2dbname	= :dut_f3_objlist_p->dut_f3_2dbname,
	    dut_f3_3owner	= :dut_f3_objlist_p->dut_f3_3owner,
	    dut_f3_5show_view	= :dut_f3_objlist_p->dut_f3_5show_view,
	    dut_f3_6show_table	= :dut_f3_objlist_p->dut_f3_6show_table,
	    dut_f3_7show_system	= :dut_f3_objlist_p->dut_f3_7show_system
	    );
	if (dut_cb->dut_c8_status == DUT_S_NEW_OBJ_SELECT)
	{
	    dut_cb->dut_c8_status = E_DU_OK;
	    dut_ff3_3reload_obj(dut_cb, dut_errcb);
	}
	EXEC FRS redisplay;
    EXEC FRS end;

    EXEC FRS activate menuitem 'Top', frskey5;
    EXEC FRS begin;
	dut_uu7_table_top(NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Bottom', frskey6;
    EXEC FRS begin;
	dut_uu8_table_bottom(NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Find', frskey7;
    EXEC FRS begin;
	dut_uu9_table_find(dut_cb, dut_errcb, DUT_M04_OBJNAME, NULL, NULL);
    EXEC FRS end;

    EXEC FRS activate menuitem 'Help', frskey1;
    EXEC FRS begin;
	EXEC FRS HELP_FRS
	    (
	    subject	= 'Distributed Database Help',
	    file	= :dut_cb->dut_helpcb->dut_hf3_1name
	    );
    EXEC FRS end;

    EXEC FRS activate menuitem 'End', frskey3;
    EXEC FRS begin;
	dut_cb->dut_c8_status = E_DU_OK;
	dut_cb->dut_c9_status = DUT_F_FORM_2;
	EXEC FRS breakdisplay;
    EXEC FRS end;
}



/*
** Name: dut_ff3_2load_obj - 
**
** Description:
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      5-dec-1988 (alexc)
**          Creation.
**     24-jul-1991 (fpang)
**	    Moved majority of code into dut_ff3_7objects().
*/
DUT_STATUS
dut_ff3_2load_obj(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    DUT_F3_OBJLIST	*dut_f3_objlist_p;
    DUT_T3_OBJLIST	*dut_f3_9objlist_p;
    DUT_T3_OBJLIST	*dut_f3_0objlist_curr_p;
    i4		dut_f3_0obj_count;
    char	dut_tmp_1ddbname[DDB_MAXNAME + 4];
    char	dut_tmp_2ddbnode[DDB_MAXNAME + 4];
    char	dut_tmp_3dbname[DDB_MAXNAME + 4];
    char	dut_tmp_4owner[DDB_MAXNAME + 4];
    char	dut_tmp_buffer[DB_LONGMSG];
    EXEC SQL END DECLARE SECTION;
    dut_f3_objlist_p = (DUT_F3_OBJLIST *)dut_cb->dut_form_objlist;

    EXEC FRS getform dut_f3_objlist
	(
	:dut_tmp_1ddbname	= dut_f3_8ddbname,
	:dut_tmp_2ddbnode	= dut_f3_1nodename,
	:dut_tmp_3dbname	= dut_f3_2dbname,
	:dut_tmp_4owner		= dut_f3_3owner
	);

    EXEC FRS getform dut_f3_objlist
	(
	:dut_f3_objlist_p->dut_f3_1nodename	= dut_f3_1nodename,
	:dut_f3_objlist_p->dut_f3_2dbname	= dut_f3_2dbname,
	:dut_f3_objlist_p->dut_f3_3owner	= dut_f3_3owner,
	:dut_f3_objlist_p->dut_f3_5show_view	= dut_f3_5show_view,
	:dut_f3_objlist_p->dut_f3_6show_table	= dut_f3_6show_table,
	:dut_f3_objlist_p->dut_f3_7show_system	= dut_f3_7show_system
	);

    dut_cb->dut_form_objlist->dut_f3_0obj_num_entry = 0;

    EXEC FRS message 'Loading Distributed Objects List';

    dut_ff3_7objects(dut_cb, dut_errcb);
    dut_ff3_3reload_obj(dut_cb, dut_errcb);

}
/*
** Name: dut_ff3_3reload_obj
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
**      5-dec-1988 (alexc)
**          Creation.
*/
DUT_STATUS
dut_ff3_3reload_obj(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    DUT_T3_OBJLIST	*dut_f3_0objlist_curr_p;
    DUT_F3_OBJLIST	*dut_f3_objlist_p;
    i4			dut_f3_0obj_count;
    i4			dut_f3_scroll;
    i4			dut_f3_current;
    EXEC SQL END DECLARE SECTION;
    i4			dut_f3_0obj_fill;

    dut_f3_current	    = 0;
    dut_f3_scroll	    = 0;
    dut_f3_0obj_count	    = dut_cb->dut_form_objlist->dut_f3_0obj_num_entry;
    dut_f3_objlist_p	    = dut_cb->dut_form_objlist;
    dut_f3_0objlist_curr_p  = dut_f3_objlist_p->dut_f3_9objlist;

    EXEC FRS clear field dut_f3_9objlist;

    EXEC FRS getform dut_f3_objlist
	(
	:dut_f3_objlist_p->dut_f3_1nodename	= dut_f3_1nodename,
	:dut_f3_objlist_p->dut_f3_2dbname	= dut_f3_2dbname,
	:dut_f3_objlist_p->dut_f3_3owner	= dut_f3_3owner,
	:dut_f3_objlist_p->dut_f3_5show_view	= dut_f3_5show_view,
	:dut_f3_objlist_p->dut_f3_6show_table	= dut_f3_6show_table,
	:dut_f3_objlist_p->dut_f3_7show_system	= dut_f3_7show_system
	);

    while (dut_f3_0obj_count--)
    {
	CVupper(dut_f3_0objlist_curr_p->dut_t3_0system);
	CVupper(dut_f3_0objlist_curr_p->dut_t3_0objtype);

	dut_uu11_chk_criteria(dut_cb, dut_errcb, dut_f3_0objlist_curr_p);

	if (dut_f3_0objlist_curr_p->dut_t3_0select &&
		(dut_f3_0objlist_curr_p->dut_t3_0delete == FALSE))
	{
	    dut_f3_current++;
	    if (!STcompare(dut_f3_0objlist_curr_p->dut_t3_1objname,
		dut_f3_objlist_p->dut_f3_9objlist_curr_p->dut_t3_1objname))
	    {
		dut_f3_scroll = dut_f3_current;
	    }
	    EXEC FRS loadtable dut_f3_objlist dut_f3_9objlist
		(
		dut_t3_0objowner= :dut_f3_0objlist_curr_p->dut_t3_0objowner,
		dut_t3_1objname	= :dut_f3_0objlist_curr_p->dut_t3_1objname,
		dut_t3_2objtype	= :dut_f3_0objlist_curr_p->dut_t3_2objtype
		);
	}
	dut_f3_0objlist_curr_p++;
    }
    if (dut_f3_scroll != 0)
    {
	EXEC FRS scroll dut_f3_objlist dut_f3_9objlist
	    to :dut_f3_scroll; 
    }

    return(E_DU_OK);
}


/*
** Name: dut_ff3_4objinfo
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
**      5-dec-1988 (alexc)
**          Creation.
*/
DUT_STATUS
dut_ff3_4objinfo(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    DUT_F3_OBJLIST	*dut_f3_objlist_p;
    DUT_T3_OBJLIST	*dut_t3_objlist_p;
    DUT_T3_OBJLIST	*dut_t3_objlist_curr_p;
    DUT_T3_OBJLIST	dut_t3_buffer;
    DUT_T3_OBJLIST	*dut_t3_p;
    EXEC SQL end declare section;

    dut_f3_objlist_p	= (DUT_F3_OBJLIST *)dut_cb->dut_form_objlist;
    dut_t3_objlist_p	= (DUT_T3_OBJLIST *)dut_f3_objlist_p->dut_f3_9objlist;

    EXEC FRS getrow dut_f3_objlist dut_f3_9objlist
	(
	:dut_t3_buffer.dut_t3_0objowner= dut_t3_0objowner,
	:dut_t3_buffer.dut_t3_1objname = dut_t3_1objname,
	:dut_t3_buffer.dut_t3_2objtype = dut_t3_2objtype
	);

    dut_uu14_find_object(dut_cb,
		dut_errcb,
		&dut_t3_p,
		dut_t3_buffer.dut_t3_0objowner,
		dut_t3_buffer.dut_t3_1objname);

    if (dut_t3_p == NULL)
    {
	return(DUT_ERROR);
    }
    
    switch(dut_t3_p->dut_t3_0system[0])
    {
    case 'Y':
    case 'N':
	switch(dut_t3_p->dut_t3_0objtype[0])
	{
	case 'T':	/* Table		*/
	    STcopy(dut_t3_p->dut_t3_0objowner,
		dut_cb->dut_popup_objattr->dut_p8_3objowner);
	    STcopy(dut_t3_p->dut_t3_1objname,
		dut_cb->dut_popup_objattr->dut_p8_2objname);
	    dut_pp8_1objattr(dut_cb, dut_errcb);
	    break;
	case 'V':	/* View			*/
	/* this may be a distributed view or a local view that has being
	** registered.
	*/
	    STcopy(dut_t3_p->dut_t3_0objowner,
		dut_cb->dut_popup_objattr->dut_p8_3objowner);
	    STcopy(dut_t3_p->dut_t3_1objname,
		dut_cb->dut_popup_objattr->dut_p8_2objname);
	    STcopy(dut_t3_p->dut_t3_0objowner,
		dut_cb->dut_popup_viewattr->dut_p9_4view_owner);
	    STcopy(dut_t3_p->dut_t3_1objname,
		dut_cb->dut_popup_viewattr->dut_p9_3view_name);
	    dut_pp9_1viewattr(dut_cb, dut_errcb);
	    break;
	case 'I':	/* Index		*/
	    STcopy(dut_t3_p->dut_t3_0objowner,
		dut_cb->dut_popup_objattr->dut_p8_3objowner);
	    STcopy(dut_t3_p->dut_t3_1objname,
		dut_cb->dut_popup_objattr->dut_p8_2objname);
	    dut_pp8_1objattr(dut_cb, dut_errcb);
/*	    dut_ppx_indexattr(dut_cb, dut_errcb);*/
	    break;
	case 'P':	/* Partitioned		*/
/*	    dut_xxx_partattr(dut_cb, dut_errcb);*/
	    break;
	case 'R':	/* Replicate		*/
/*	    dut_xxx_replicateattr(dut_cb, dut_errcb);*/
	    break;
	default:
	    /* error occur */
	    break;
	}
    default:
	/* error occur */
	break;
    }
    return(E_DU_OK);
}

/*
** Name: dut_ff3_6remove_object
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
**      5-dec-1988 (alexc)
**          Creation.
*/
DUT_STATUS
dut_ff3_6remove_object(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL begin declare section;
    DUT_F3_OBJLIST	*dut_f3_objlist_p;
    DUT_T3_OBJLIST	*dut_t3_objlist_p;
    DUT_T3_OBJLIST	*dut_t3_objlist_curr_p;
    char		dut_stmt_buffer[DB_LONGMSG];
    char		dut_confirm[DDB_MAXNAME];
    EXEC SQL end declare section;
    i4			dut_t3_found;
    i4			dut_t3_count;

    dut_t3_found	= FALSE;
    dut_f3_objlist_p	= 
	(DUT_F3_OBJLIST *)dut_cb->dut_form_objlist;
    dut_t3_objlist_p	= 
	(DUT_T3_OBJLIST *)dut_cb->dut_form_objlist->dut_f3_9objlist;
    dut_t3_objlist_curr_p = 
	(DUT_T3_OBJLIST *)dut_cb->dut_form_objlist->dut_f3_9objlist_curr_p;
    dut_t3_count	= dut_f3_objlist_p->dut_f3_0obj_num_entry;

    while (!dut_t3_found && dut_t3_count--)
    {
	if (!STcompare(dut_t3_objlist_p->dut_t3_1objname,
			dut_t3_objlist_curr_p->dut_t3_1objname)
			&&
	    !STcompare(dut_t3_objlist_p->dut_t3_2objtype,
			dut_t3_objlist_curr_p->dut_t3_2objtype)
			&&
	    !STcompare(dut_t3_objlist_p->dut_t3_0objowner,
			dut_t3_objlist_curr_p->dut_t3_0objowner)
			&&
	    (dut_t3_objlist_p->dut_t3_0delete == FALSE)
	    )
	{
	    dut_t3_found = TRUE;
	    break;
	}
	else
	{
	    dut_t3_objlist_p++;
	}
    }

    if (dut_t3_found == TRUE)
    {
	dut_t3_objlist_curr_p->dut_t3_0select 
		= dut_t3_objlist_p->dut_t3_0select;
	dut_t3_objlist_curr_p->dut_t3_0objbase
		= dut_t3_objlist_p->dut_t3_0objbase;
	STcopy(dut_t3_objlist_p->dut_t3_0objtype,
		dut_t3_objlist_curr_p->dut_t3_0objtype);
	STcopy(dut_t3_objlist_p->dut_t3_0subtype,
		dut_t3_objlist_curr_p->dut_t3_0subtype);
	STcopy(dut_t3_objlist_p->dut_t3_0system, 
		dut_t3_objlist_curr_p->dut_t3_0system);
	STcopy(dut_t3_objlist_p->dut_t3_1objname, 
		dut_t3_objlist_curr_p->dut_t3_1objname);
	STcopy(dut_t3_objlist_p->dut_t3_2objtype, 
		dut_t3_objlist_curr_p->dut_t3_2objtype);
	STcopy(dut_t3_objlist_p->dut_t3_0objowner,
		dut_t3_objlist_curr_p->dut_t3_0objowner);
    }
    else
    {
	/* Added error message here */
	return(DUT_ERROR);
    }

    switch(dut_t3_objlist_curr_p->dut_t3_0system[0])
    {
    case 'Y':
	/* Does not allow removal of System object */
	break;
    case 'N':
	switch(dut_t3_objlist_curr_p->dut_t3_0objtype[0])
	{
	case 'V':	/* Registered View	*/
	case 'T':	/* Table		*/
	    if (dut_t3_objlist_curr_p->dut_t3_0objtype[0] == 'V' && 
		dut_t3_objlist_curr_p->dut_t3_0subtype[0] == 'N')
	    {
		/* Do not want to remove if it is a Star-level View */
	        dut_ee1_error(dut_errcb, W_DU1819_SV_RM_NONE_REG_OBJ, 0);
		break;
	    }
	    STcopy("U", dut_confirm);
	    while (STcompare(dut_confirm, "y") && STcompare(dut_confirm, "n"))
	    {
		STprintf(dut_cb->frs_message,
			DUT_M06_CONFIRM_REMOVE,
			dut_t3_objlist_curr_p->dut_t3_1objname);
		EXEC FRS prompt
		    (
		    :dut_cb->frs_message,
		    :dut_confirm
		    );
		CVlower(dut_confirm);
	    }
	    if (!STcompare(dut_confirm, "n") || !STcompare(dut_confirm, ""))
	    {
		return(E_DU_OK);
	    }

	    STprintf(dut_stmt_buffer,
			DUT_M07_REMOVING_DOBJ,
			dut_t3_objlist_curr_p->dut_t3_1objname);
	    EXEC FRS message :dut_stmt_buffer;

	    if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
	    {
		dut_ee1_error(dut_errcb, W_DU1805_SV_CANT_OPEN_DDB, 2, 0, 
				dut_cb->dut_c1_ddbname);
	    }

	    STprintf(dut_stmt_buffer,
		"remove %s %s",
		( dut_t3_objlist_curr_p->dut_t3_0objtype[0] == 'V' ?
		"view" : (dut_t3_objlist_curr_p->dut_t3_0objtype[0] == 'T' ?
			"table" : ""
			)
		),
		dut_t3_objlist_curr_p->dut_t3_1objname);

	    EXEC SQL commit;
	    EXEC SQL execute immediate :dut_stmt_buffer;
	    if (sqlca.sqlcode < 0)
	    {
		DUT_STATUS msg_no;

		switch(dut_t3_objlist_curr_p->dut_t3_0objtype[0])
		{
		case 'V':	/* Link or Register	*/
			msg_no = W_DU1814_SV_CANT_DROP_TABLE;
			break;
		case 'T':	/* Table		*/
			msg_no = W_DU1815_SV_CANT_DROP_VIEW;
			break;
		default:
			msg_no = W_DU1819_SV_RM_NONE_REG_OBJ;
			break;
		}
	        dut_ee1_error(dut_errcb, msg_no,
			2, 0, dut_t3_objlist_curr_p->dut_t3_1objname);
	    }
	    else
	    {
		dut_t3_objlist_curr_p->dut_t3_0delete = TRUE;
		dut_t3_objlist_p->dut_t3_0delete = TRUE;
	    }
	    EXEC SQL commit;
	    dut_uu18_chk_ldblist(dut_cb, dut_errcb);
	    dut_ff3_8fixlist(dut_cb, dut_errcb);
	    dut_ff3_3reload_obj(dut_cb, dut_errcb);
	    break;
	case 'I':	/* Index		*/
	    dut_ee1_error(dut_errcb, W_DU1819_SV_RM_NONE_REG_OBJ, 0);
	    break;
	case 'P':	/* Partitioned		*/
	case 'R':	/* Replicate		*/
	default:
	    /* Call error for object is not a register object */
	    /* Do not drop anything but registered tables */
	    dut_ee1_error(dut_errcb, W_DU1819_SV_RM_NONE_REG_OBJ, 0);
	    break;
	}
	break;
    default:
	/* error occur */
	dut_ee1_error(dut_errcb, W_DU1819_SV_RM_NONE_REG_OBJ, 0);
	break;
    }
    return(E_DU_OK);
}


/*
** Name: dut_ff3_7objects
**
** Description:
**  Load objects list into objects list array.
**
** History:
**      24-jul-1991 (fpang)
**          Created from dut_ff3_2load_obj().
**      10-12-94 (newca01) 
**          added new union to select to get star views for B48950
*/
DUT_STATUS
dut_ff3_7objects(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    EXEC SQL BEGIN DECLARE SECTION;
    DUT_F3_OBJLIST	*dut_f3_objlist_p;
    DUT_T3_OBJLIST	*dut_f3_9objlist_p;
    DUT_T3_OBJLIST	*dut_f3_0objlist_curr_p;
    i4		dut_f3_0obj_count;
    EXEC SQL END DECLARE SECTION;
    i4		dut_f3_0obj_fill;

    dut_f3_0obj_count = 0;
    dut_f3_0obj_fill  = 0;
    dut_f3_objlist_p = (DUT_F3_OBJLIST *)dut_cb->dut_form_objlist;
    dut_f3_9objlist_p = (DUT_T3_OBJLIST *)dut_f3_objlist_p->dut_f3_9objlist;

    if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU1805_SV_CANT_OPEN_DDB, 2, 0, 
                      dut_cb->dut_c1_ddbname);
    }

    EXEC SQL select
	count(*) into :dut_f3_0obj_count
	from iiddb_objects, iitables
	where iiddb_objects.object_name = iitables.table_name
		and
	      iiddb_objects.object_owner = iitables.table_owner;

    if (dut_f3_0obj_count > dut_cb->dut_form_objlist->dut_f3_0obj_size)
    {
	MEfree(dut_f3_9objlist_p);
	dut_f3_objlist_p->dut_f3_9objlist = NULL;
	dut_f3_9objlist_p = NULL;
    }

    if (dut_f3_9objlist_p == NULL)
    {
	dut_f3_9objlist_p = (DUT_T3_OBJLIST *)MEreqmem(0,
		(dut_f3_0obj_count + DUT_ME_EXTRA) * sizeof(DUT_T3_OBJLIST),
		TRUE, NULL);
	dut_f3_objlist_p->dut_f3_9objlist = dut_f3_9objlist_p;
	dut_cb->dut_form_objlist->dut_f3_0obj_size 
		= dut_f3_0obj_count + DUT_ME_EXTRA;
    }

    dut_f3_0objlist_curr_p = dut_f3_9objlist_p;

    EXEC SQL 
	SELECT
	    iiddb_objects.object_base,
	    lowercase(trim(iiddb_objects.object_name)),
	    lowercase(trim(iiddb_objects.object_owner)),
	    lowercase(trim(iitables.table_type)),
	    lowercase(trim(iitables.table_subtype)),
	    lowercase(trim(iiddb_objects.system_object)),
	    lowercase(trim(iiddb_ldbids.ldb_database)),
	    lowercase(trim(iiddb_ldbids.ldb_node))
	into
	    :dut_f3_0objlist_curr_p->dut_t3_0objbase,
	    :dut_f3_0objlist_curr_p->dut_t3_1objname,
	    :dut_f3_0objlist_curr_p->dut_t3_0objowner,
	    :dut_f3_0objlist_curr_p->dut_t3_0objtype,
	    :dut_f3_0objlist_curr_p->dut_t3_0subtype,
	    :dut_f3_0objlist_curr_p->dut_t3_0system,
	    :dut_f3_0objlist_curr_p->dut_t3_0ldbname,
	    :dut_f3_0objlist_curr_p->dut_t3_0ldbnode
	from
	    iiddb_objects, iiddb_tableinfo, iiddb_ldbids, iitables
	where 
	    (iiddb_objects.object_base = iiddb_tableinfo.object_base)
		and
	    (iiddb_objects.object_index = iiddb_tableinfo.object_index)
		and
	    (iitables.table_name = iiddb_objects.object_name)
		and
	    (iitables.table_owner = iiddb_objects.object_owner)
	        and
	    (iiddb_ldbids.ldb_id = iiddb_tableinfo.ldb_id)

        union 
	select  
	    iiddb_objects.object_base,
	    lowercase(trim(iiddb_objects.object_name)),
	    lowercase(trim(iiddb_objects.object_owner)),
	    lowercase(trim(iitables.table_type)),
	    lowercase(trim(iitables.table_subtype)),
	    lowercase(trim(iiddb_objects.system_object)),
	    ' ', 
	    ' '                                
	from
	    iiddb_objects, iitables
	where 
	    (iitables.table_name = iiddb_objects.object_name)
		and
	    (iitables.table_owner = iiddb_objects.object_owner)
	        and
	    (iiddb_objects.object_type = 'V')
        order by 2, 3;
    EXEC SQL begin;
	CVupper(dut_f3_0objlist_curr_p->dut_t3_0system);
	CVupper(dut_f3_0objlist_curr_p->dut_t3_0objtype);
	CVupper(dut_f3_0objlist_curr_p->dut_t3_0subtype);

	if (dut_cb->dut_form_objlist->dut_f3_0obj_num_entry == 0)
	{
	    /* 0 indicates user requested info ( select ListObj )
	    ** as opposed to re-fetch of info because we detected
	    ** that new objects exist or cease to exist as a result
	    ** of a 'Remove' or 'Register'.
	    ** Give informational message if the former.
	    */

	    STprintf(dut_cb->frs_message,
		    DUT_M05_LOAD_DOBJ,
		    dut_f3_0objlist_curr_p->dut_t3_1objname);

	    EXEC FRS message :dut_cb->frs_message;
	}

	dut_f3_0objlist_curr_p->dut_t3_0delete = FALSE;

	dut_uu11_chk_criteria(dut_cb, dut_errcb, dut_f3_0objlist_curr_p);

	dut_f3_0obj_fill++;

	if (dut_f3_0obj_fill > dut_cb->dut_form_objlist->dut_f3_0obj_size)
	{
	    EXEC SQL endselect;
	}
	dut_f3_0objlist_curr_p++;

	if (sqlca.sqlcode < 0)
	{
	    dut_cb->dut_c8_status = !E_DU_OK;
	    dut_ee1_error(dut_errcb, W_DU1807_SV_CANT_GET_DDB_INFO, 2,
			0, dut_cb->dut_c1_ddbname);
	    EXEC SQL endselect;
	}
    EXEC SQL end;

    dut_uu13_buble_sort(dut_cb->dut_form_objlist->dut_f3_9objlist,
			sizeof(DUT_T3_OBJLIST), DDB_MAXNAME,
			dut_f3_0obj_fill);


    if (dut_cb->dut_form_objlist->dut_f3_0obj_num_entry == 0)
    {
	/* See comment above regarding informational message.
	** This code here to place the cursor on the first entry
	** if we get here via a 'ListObj', and current otherwise.
	*/

        dut_f3_0objlist_curr_p = dut_f3_objlist_p->dut_f3_9objlist_curr_p;

        MEcopy(dut_f3_objlist_p->dut_f3_9objlist, sizeof(DUT_T3_OBJLIST),
	       dut_f3_0objlist_curr_p);
    }

    dut_cb->dut_form_objlist->dut_f3_0obj_num_entry = dut_f3_0obj_fill;

    return(E_DU_OK);
}


/*
** Name: dut_ff3_8fixlist
**
** Description:
**	Set current to point to next object after the one just deleted.
**
** History:
**      24-jul-1991 (fpang)
**          Created.
*/
DUT_STATUS
dut_ff3_8fixlist(dut_cb, dut_errcb)
EXEC SQL BEGIN DECLARE SECTION;
DUT_CB	    *dut_cb;
DUT_ERRCB   *dut_errcb;
EXEC SQL END DECLARE SECTION;
{
    DUT_T3_OBJLIST	*pp;
    DUT_F3_OBJLIST	*dut_f3_objlist_p;
    DUT_T3_OBJLIST	*curr_p;
    DUT_T3_OBJLIST	curr;
    i4		dut_f3_0obj_count;


    /* Reload objects */
    dut_ff3_7objects(dut_cb, dut_errcb);

    dut_f3_0obj_count = dut_cb->dut_form_objlist->dut_f3_0obj_num_entry;
    dut_f3_objlist_p = (DUT_F3_OBJLIST *)dut_cb->dut_form_objlist;
    pp = (DUT_T3_OBJLIST *)dut_f3_objlist_p->dut_f3_9objlist;
    curr_p = dut_cb->dut_form_objlist->dut_f3_9objlist_curr_p;

    if (dut_uu2_dbop(DUT_DDBOPEN, dut_cb, dut_errcb) != E_DU_OK)
    {
	dut_ee1_error(dut_errcb, W_DU1805_SV_CANT_OPEN_DDB, 2, 0, 
                      dut_cb->dut_c1_ddbname);
    }

    /* Find new current */
    while (dut_f3_0obj_count--)
    {
	if (STcompare(curr_p->dut_t3_1objname, pp->dut_t3_1objname) <= 0)
	{
	    MEcopy(pp, sizeof(DUT_T3_OBJLIST), curr_p);
	    break;
	}
	pp++;
    }

    if (dut_f3_0obj_count == -1)
    {
	pp--;
        MEcopy(pp, sizeof(DUT_T3_OBJLIST), curr_p);
    }

    return(E_DU_OK);
}
