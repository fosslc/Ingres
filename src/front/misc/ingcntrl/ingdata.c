/*
** Copyright (c) 2004 Ingres Corporation
*/

#include        <compat.h>
#include        <er.h>
#include        <gl.h>
#include        <sl.h>
#include        <iicommon.h>
#include        <fe.h>
#include        <uf.h>
#include        <ui.h>
#include        "ictables.h"

/*
** Name:	ingdata.c
**
** Description:	Global data for ingcntrl facility.
**
** History:
**
**	23-sep-96 (mcgem01)
**	    Created.
**	21-aug-97 (mosjo01)
**	    Had to change uname to zname (or rather, use anything but uname)
**	    so that sos_us5 (sco openserver) compiler/linker would resolve
**	    the true address to gethostname(uname function) instead of 
**	    doing a call into the middle of a data area. This was occuring
**	    in ingcntrl (accessdb/catalogdb).  This became a bizzare problem
**	    when uname became a GLOBALREF in ingcntrl.qsc.
**	16-Apr-98 (merja01)
**		Change uname to ii_uname.  The uname symbol caused an evasive
**		illegal instruction error to occur on axp_osf and had 
**		previously created problems on sos_us5 as well.  I have removed
**		the ifdef for sos_us5 to make this change generic.  This
**		change corrects Digital UNIX 4.0D bug 90041.
**      07-Jan-99 (hanal04)
**          Added IiicPrivMntAudit to record whether the user has the
**          maintain_audit privilege. b81618. 
*/


/* ingcntrl.qsc */

GLOBALDEF bool  Mgr = FALSE;
GLOBALDEF char  *Uover = ERx("");
GLOBALDEF char  *Pswd = ERx("");
GLOBALDEF char  *IC_allDBs;
GLOBALDEF char  *IC_PublicUser;

GLOBALDEF char  *Yes;
GLOBALDEF char  *No;
GLOBALDEF char  *Request;

GLOBALDEF bool  IiicPrivSysadmin;
GLOBALDEF bool  IiicPrivSecurity;
GLOBALDEF bool  IiicPrivMaintUser;
GLOBALDEF bool  IiicC2Security; 
/* b81618 */
GLOBALDEF bool  IiicPrivMntAudit;
GLOBALDEF char  ii_uname[FE_MAXNAME+1];
GLOBALDEF char  *Real_username = &ii_uname[0];
GLOBALDEF char   F_Maxname[] = ERx("c32");


/* selects.qsc */

GLOBALDEF IIUSER Iiuser;
GLOBALDEF IIUSERGROUP Iiusergroup;
GLOBALDEF IIROLE Iirole;
GLOBALDEF IIDBPRIV Iidbpriv;
GLOBALDEF IIDATABASE Iidatabase;

/* user.qsc */

GLOBALDEF char Usrform[] = ERx("usrfrm");
GLOBALDEF char Pwdform[] = ERx("pwdfrm");
GLOBALDEF char Prvform[] = ERx("prvfrm");
