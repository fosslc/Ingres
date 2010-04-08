/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<lqgdata.h>

/**
+*  Name: iife.c - Support some front-end interfaces
**
**  Description:
**	This file supports some front-end special calls.
**
**  Defines:
**	IIlq_Protect 	- Send protection block to DBMS.
**	IIfeddb		- Is DBMS distributed (phasing out).
**	IIfePrepare	- Distributed PREPARE statement (phasing out).
**	IIfeDescribe	- Distributed DESCRIBE statement (phasing out).
**
**  Notes:
-*
**  History:
**	13-apr-1987	- Written (ncg)
**	26-aug-1987 	- Modified to use GCA. (ncg)
**	11/02/90 (dkh) - Added routine IILQgdata() to replace IILIBQgdata
**			 as the interface for the outside world to use
**			 for accessing libq global data.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	01-apr-1999 (somsa01)
**	    Slight adjustment to declaration of IIlq_Protect() to supress
**	    building headaches on some UNIX platforms.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF	LIBQGDATA	*IILIBQgdata;

/*{
+*  Name: IIlq_Protect - Send "protect" block to DBMS.
**
**  Description:
**	This routine is called when user applications are running our front-ends
**	that want to dynamically turn on protected system catalog updates.
**
**  Inputs:
**	set_flag	- Usage of protection mode (1/0 - On/Off).
**
**  Outputs:
**	Returns:
**	    Void
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	13-apr-1987	- Written (ncg)
**	26-aug-1987 	- Modified to use GCA. (ncg)
*/

void
IIlq_Protect(bool set_flag)
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    DB_DATA_VALUE	dbv;
    i4		index = GCA_SUPSYS;
    i4		value = set_flag ? GCA_ON : GCA_OFF;
    i4		count = 1;

    if (IIqry_start(GCA_MD_ASSOC, 0, 0, ERx("protect")) != OK)
	return;

    II_DB_SCAL_MACRO(dbv, DB_INT_TYPE, sizeof(count), &count);
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VVAL, &dbv);
    II_DB_SCAL_MACRO(dbv, DB_INT_TYPE, sizeof(index), &index);
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VVAL, &dbv);
    II_DB_SCAL_MACRO(dbv, DB_INT_TYPE, sizeof(value), &value);
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VDBV, &dbv);
    IIsyncup((char *)0, 0);
}

/*{
** Name:	IIfeddb	-	Return whether connected to ddb.
**
** Description:
**	This routine returns whether the FE is connected to a DDB.
**
** Outputs:
**	Returns:
**		0	if not connected to DDB.
**		!=0	if connected to DDB.
*/
i4
IIfeddb()
{
    return 0;
}

i4
IIfePrepare()
{
    return -1;
}
    
i4
IIfeDescribe()
{
    return -1;
}



/*{
** Name:	IILQgdata - Return value of IILIBQgdata.
**
** Description:
**	This routine simply returns the value of IILIBQgdata.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		IILIBQgdata
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	11/02/90 (dkh) - Initial version.
*/
LIBQGDATA *
IILQgdata()
{
	return(IILIBQgdata);
}

/* Name: IIhelptoggle -- Toggle the II_L_HELP bit according to
**           the status of 'help ...' statement that is 
**           being run within the 'isql' session.
**
** Description:
**     This routine toggle the II_L_HELP bit. It was turned
**     ON after user runs a 'help ...' statement within 
**     a 'ISQL' session and was turned OFF after such 
**    'help' statement runs to coompletion. This routine
**     is used to prevent 'ISQL' send out a GCA_INTALL 
**     message if user press the END key before pass the 
**     output of 'help' statement. On a session
**     with 'on_error = rollback transaction', sending out
**     a GCA_INTALL could cause server to rollback the
**     whole transaction. 
**      
** History: 
**     19-oct-2004 (huazh01)
**        written. 
** 24/04/07 (kiria01) b118184
**     Renamed IIhelp to IIhelptoggle to avoid naming conflict
**     with runsys routines.
*/
void
IIhelptoggle(bool flag)
{
   II_THR_CB	*thr_cb = IILQthThread();
   II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session; 

   if (flag == TRUE)
   {
       IIlbqcb->ii_lq_flags |= II_L_HELP;  
   }
   else
   {
      IIlbqcb->ii_lq_flags &= ~II_L_HELP; 
   }
   return; 
}


