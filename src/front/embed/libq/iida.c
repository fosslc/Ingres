/*
** Copyright (c) 2004 Ingres Corporation
*/

#include        <compat.h>
#include        <me.h>          /* 6-x_PC_80x86 */
#include        <pc.h>          /* 6-x_PC_80x86 */
#include        <si.h>
#include        <er.h>
#include        <cm.h>
#include        <st.h>
# include	<gl.h>
# include	<sl.h>
#include	<cv.h>
# include	<iicommon.h>
#include        <adf.h>
#include        <generr.h>
#include	<eqrun.h>
# include	<iisqlca.h>
#include        <iilibq.h>
#include        <erlq.h>
#include        <sqlstate.h>    /* needed for SQLSTATE defines */
#include	<fe.h>

/*{
+*  Name:  iida.c	- ANSI SQL Diagnostic Area Support.
**      The routines in this file are used to set/reset/retrieve info
**	from the SQL Diagnostic Area.  They can be called by a LIBQ module,
**	or by LIBQGCA through the gca handler in iigcahdl.c.
**
**  Defines:
**	IILQdaSIsetItem		- Set an Information Item in the DA.
**
**  Notes:
-*
**  History:
**      03-nov-92 (larrym)
**          Written.
**	07-jan-93 (teresal)
**	    Replace call to FEadfcb() with new LIBQ routine IILQasAdfcbSetup()
**	    which allows multiple ADF control blocks (i.e., one for each
**	    session).
**	05-feb-93 (larrym)
**	    Fixed "bug" where we don't bother setting SQLSTATE in the DA 
**	    if the users application doesn't reference it.  This behavior is 
**	    ok for now, but when we add support for GET DIAGNOSTIC, it would
**	    have become a bug.  
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	aug-2009 (stephenb)
**		Prototyping front-ends
*/

/*{
**  Name: IILQdaSIsetItem	- Set an Information Item in the DA.
**
**  Description:
**
**  Inputs:
**	thr_cb		Thread-local-storage control block.
**	itemtype	- type of Diagnostic information passed in.
**	item		- (char *) to Diagnostic Data.
**
**  Outputs:
**      Returns:
**          Nothing
**      Errors:
**          None
**
**  Side Effects:
**  1.  This function is sometimes called by IIlocerr, and this function can
**	also call IIlocerr, which could lead to recursion.  As this function
**	is now written, recursion is prevented.  But you need to be aware of
**	this if you are modifying it.  See the note by the IIlocerr call for 
**	more info.
**
**  History:
**
**      03-nov-92 (larrym)
**          Written.
**	09-dec-92 (teresal)
**	    Modified ADF control block reference to be session specific
**	    instead of global (part of decimal changes).
**	11-feb-1993 (larrym)
**	    SQLSTATE is not null terminated.  Changed STlcopy to MEcopy.
*/

VOID
IILQdaSIsetItem( II_THR_CB *thr_cb, u_i4 da_itemtype, char *da_item )
{

    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    DB_DATA_VALUE       dbiq;		/* points to internal SQLSTATE */
    DB_EMBEDDED_DATA    edv;		/* points to User's host var */
    DB_STATUS           stat;		/* STATUS, like we really care :-) */
    i2      		indaddr = 0;	/* address of null indicator */
    IILQ_DA		*iida;		/* pointer to diagnostic area */
    char        	ebuf[20];	/* won't need it. */

    iida = &thr_cb->ii_th_da;

    switch(da_itemtype)
    {
    case IIDA_RETURNED_SQLSTATE:

	/* 
	** Don't check for dml == SQL here.  Outside of connections, the dml
	** is set to QUEL, and we need to set SQLSTATEs for failed connections.
	** (In otherwords, I've already made that mistake).
	*/

	/* Anything to set? */
	if (da_item == (PTR)0)
	    break;

	/* 
	** we're either Setting an SQLSTATE, in which case we don't want
	** to overwrite a previous SQLSTATE, or we're REsetting the SQLSTATE
	** to it's initial (SUCCESS) state, in which case we overwrite with
	** reckless abandon.  While we're doing all this checking, we may
	** as well set the flags.
	*/
	if (STcompare(da_item, SS00000_SUCCESS)) 
	{
	    /* da_item is *not* SUCCESS */
	    if (iida->ii_da_infostate & II_DA_RS_SET)
	    {
		break; 	/* SQLSTATE is already set. don't overwrite */
	    }
	    else
	    {
		/* setting */
		iida->ii_da_infostate |= II_DA_RS_SET; 
	    }
	}
	else
	{
	    /* da_item *is* SUCCESS */
	    /* Reseting */ 
	    iida->ii_da_infostate &= ~II_DA_RS_SET;
	}

	/* in either case there is now a valid value in the DA */
	iida->ii_da_infostate |= II_DA_SET;
		
	/* Copy SQLSTATE into DA */
	MEcopy (da_item, DB_SQLSTATE_STRING_LEN, iida->ii_da_ret_sqlst);

	/* Done with the DA stuff, now Copy SQLSTATE into User's SQLSTATE */
	/* If user isn't using SQLSTATE, return. */
	if (iida->ii_da_usr_sqlsttype == T_NONE)
	    break;

	dbiq.db_prec = 0;
	dbiq.db_datatype = DB_CHA_TYPE;
	dbiq.db_length = DB_SQLSTATE_STRING_LEN;
	dbiq.db_data = da_item;

	/* Make sure there is an ADF_CB set up */
	if (IIlbqcb->ii_lq_adf == NULL)
	    if (IILQasAdfcbSetup( thr_cb ) != OK)
		return;

	/* Build an EDV from caller's embedded variable information */
	edv.ed_type = (i2)iida->ii_da_usr_sqlsttype;
	edv.ed_length = (i4)DB_SQLSTATE_STRING_LEN;
	edv.ed_data = (PTR)iida->ii_da_usr_sqlstdata;
	edv.ed_null = &indaddr;

	/* Take this SQLSTATE and shove it (into the user's EV) */
	stat = adh_dbcvtev(IIlbqcb->ii_lq_adf, &dbiq, &edv);
	if (stat != E_DB_OK)
	{
	    /* IIlocerr calls this function, so you'd think this
	    ** could recurse on us, but it shouldn't as once an
	    ** SQLSTATE is set, it is not overwritten (we would 
	    ** return up at the top and not get down here).  But we
	    ** need to watch out for this if we start changing the 
	    ** code around.
	    */
	    IIlocerr((i4)(GE_DATA_EXCEPTION + GESC_ASSGN),
	             E_LQ00A4_ATTVALTYPE, II_ERR, 0, (char *)0);
	}
	break;

    default:
	/* Bad ItemType (no biscuit) */
	CVlx(da_itemtype, ebuf);
	IIlocerr(GE_LOGICAL_ERROR,E_LQ0045_DA_INV_ITEM_TYPE, II_ERR, 1, ebuf);
	break;
    } /* switch */

    return;
}
