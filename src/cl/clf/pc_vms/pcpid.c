/*
** Copyright 1983, 2008 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<efndef.h>
# include	<iledef.h>
# include	<iosbdef.h>
# include	<jpidef.h>
# include	<pc.h>
# include	"pclocal.h"
# include	<ssdef.h>
# include	<me.h>
# include	<st.h>
# include	<starlet.h>

/*
**	PCpid(pidp) -- return process id of caller in *pidp.
**
**	History:
**		09/83 (wood) -- changed from macro to routine for VMS.
**		12-apr-84 (fhc) -- modified to be efficient.  process id's
**				don't change, so only need to call this once
**		2-oct-84 (dreyfus) -- cleaned up initilization. Include
**			jpidef.h instead of vmsjpi.h.
**	8-jun-93 (ed)
**	    add STATUS return code for prototype
**
**      16-jul-93 (ed)
**	    added gl.h, changed PCpid to not return status, added me.h st.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	26-oct-2001 (kinte01)
**	   Clean up compiler warnings
**	29-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	27-jul-2004 (abbjo03)
**	    Rename pc.h to pclocal.h to avoid conflict with CL hdr!hdr file.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	28-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**/

void
PCpid(
PID	*pidp)
{
	static	PID	holdpid = 0;
	static	bool	got_pid = FALSE;
	ILE3		itemlist[2];
	IOSB		iosb;

	PCstatus = OK;
	if (!got_pid)
	{
		itemlist[0].ile3$w_length = sizeof(PID);
		itemlist[0].ile3$w_code = JPI$_PID;
		itemlist[0].ile3$ps_bufaddr = (PTR)&holdpid;
		itemlist[0].ile3$ps_retlen_addr = NULL;

		itemlist[1].ile3$w_length = NULL;
		itemlist[1].ile3$w_code = NULL;
		itemlist[1].ile3$ps_bufaddr = NULL;
		itemlist[1].ile3$ps_retlen_addr = NULL;

		PCstatus = sys$getjpiw(EFN$C_ENF, 0, 0, &itemlist, &iosb, 0, 0);
		if (PCstatus & 1)
		    PCstatus = iosb.iosb$w_status;
		if (PCstatus == SS$_NORMAL)
		{
			PCstatus = OK;
			got_pid = TRUE;
		}
	}
	MEcopy((PTR)&holdpid, (u_i2)sizeof(PID), (PTR)pidp);
}

/* BEGIN COMPATIBILITY (azad, tim) */
/* we need to use the shortest unique subset of the 
** VMS pid for identifying temp files because of
** name length problems.  -tw 
*/
void
PCunique(
char	*numb)
{
	static	char	uniq_str[8] = "\0";
	static	bool	got_uniq = FALSE;
	PID	numbuf;

	if (!got_uniq)
	{
		PCpid(&numbuf);
		numbuf &= 0177777;	/* lower half is unique in VMS V3.x */
		STprintf(uniq_str, "%x", numbuf);
		got_uniq = TRUE;
	}
	STcopy(uniq_str, numb);
}
/* END COMPATIBILITY */
